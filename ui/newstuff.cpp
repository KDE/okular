/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qfile.h>
#include <qwidget.h>
#include <qtimer.h>
#include <qlayout.h>
#include <khbox.h>
#include <kvbox.h>
#include <qpixmap.h>
#include <qfont.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qrect.h>
#include <qpainter.h>
#include <qapplication.h>
#include <ktoolinvocation.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kconfig.h>
#include <kpushbutton.h>
#include <kstandarddirs.h>
#include <kstdguiitem.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <khtml_part.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <knewstuff/provider.h>
#include <knewstuff/entry.h>

// local includes
#include "newstuff.h"

// define the providers.xml location
#define PROVIDERS_URL "http://kpdf.kde.org/newstuff/providers.xml"
// define the type of the stuff returned by providers (unused)
#define RES_TYPE "kpdf/stuff"

/* How the whole thing works:
 - phase 1: fetch the ProvidersList from the PROVIDERS_URL
 - phase 2: every Provider in it contains some AvailableItem(s)
 - phase 3: every AvailableItem can be downloaded (aka installed) or deleted
*/

/** GUI: Internal Widget that displays a ver-stretchable (only) image */
class ExtendImageWidget : public QWidget
{
    public:
        ExtendImageWidget( const QPixmap & pix, QWidget * parent )
            : QWidget( parent ), m_pixmap( pix )
        {
            // adjust size hint
            setFixedWidth( pix.width() );
            setMinimumHeight( pix.height() );
            // paint all own pixels
            setAttribute( Qt::WA_OpaquePaintEvent, true );
            // create the tile image from last line of pixmap
            m_tile = QPixmap( pix.width(), 1 );
            QPainter p(&m_tile);
            p.drawPixmap(0, 0, pix, 0,pix.height()-2, pix.width(),1 );
        }

        // internal paint function
        void paintEvent( QPaintEvent * e )
        {
            // extend the bottom line of the image when painting
            QRegion paintRegion = e->region();
            QRect pixmapRect = e->rect().intersect( m_pixmap.rect() );
            // paint the pixmap rect
            QPainter p( this );
            p.drawPixmap( pixmapRect.topLeft(), m_pixmap, pixmapRect );
            // paint the tiled bottom part
            QVector<QRect> rects = paintRegion.subtract( pixmapRect ).rects();
            for ( int i = 0; i < rects.count(); i++ )
            {
                const QRect & tileRect = rects[ i ];
                p.drawTiledPixmap( tileRect, m_tile, QPoint(tileRect.left(), 0) );
            }
        }

    private:
        QPixmap m_pixmap;
        QPixmap m_tile;
};


/** CORE: extend KNS::Entry adding convenience methods */
class AvailableItem : public Entry
{
    public:
        typedef QList< AvailableItem * > List;
        enum State { Normal = 0, Installing = 1, Uninstalling = 2 };

        AvailableItem( const QDomElement & element )
            : Entry( element ), m_state( Normal ), m_progress( 0 )
        {
            QString remoteUrl = payload().url();
            QString fileName = remoteUrl.section( '/', -1, -1 );
            QString extension = fileName.toLower().section( '.', -1, -1 );
            QString typeString = category();
            // place books on the desktop
            if ( typeString == "kpdf/book" )
                m_destinationFile = KGlobalSettings::desktopPath() + '/' + fileName;
            // place kpdf data on the local share/apps/kpdf/stuff
            else if ( typeString == "kpdf/tool" )
                m_destinationFile = KStandardDirs::locateLocal( "data", "okular/stuff/" + fileName );
            // warn about unrecognized type
            else kDebug() << "NewStuffDialog: AvailableItem: unrecognized provider name: '"
                << typeString << "'." << endl;
        }

        // returns the source url
        QString url() const { return payload().url(); }

        // returns local destination path for the item
        const QString & destinationPath() { return m_destinationFile; }

        // checks if the item is already installed
        bool installed() { return QFile::exists( m_destinationFile ); }

        // state
        void setState( State s ) { m_state = s; }
        State state() { return m_state; }

        // progress (state dependant)
        void setProgress( float p ) { m_progress = p; }
        float progress() { return m_progress; }

    private:
        QString m_destinationFile;
        State m_state;
        float m_progress;
};


/** GUI/CORE: HTML Widget to operate on AvailableItem::List */
class ItemsView : public KHTMLPart
{
    public:
        ItemsView( NewStuffDialog * newStuffDialog, QWidget * parentWidget )
            : KHTMLPart( parentWidget ),
            m_newStuffDialog( newStuffDialog ), m_sorting( 0 )
        {
            // customize functionality
            setJScriptEnabled( true );
            setJavaEnabled( false );
            setMetaRefreshEnabled( false );
            setPluginsEnabled( false );
            setStandardFont( QApplication::font().family() );
            // 100% is too large! less is better
            setZoomFactor( 70 );
        }

        ~ItemsView()
        {
            clear();
        }

        void setItems( const AvailableItem::List & itemList )
        {
            clear();
            m_items = itemList;
            buildContents();
        }

        void setSorting( int sortType )
        {
            m_sorting = sortType;
            buildContents();
        }

        void updateItem( AvailableItem * item )
        {
            // get item id string and iformations
            QString idString = QString::number( (unsigned long)item );
            AvailableItem::State state = item->state();
            bool showProgress = state != AvailableItem::Normal;
            int pixelProgress = showProgress ? (int)(item->progress() * 80.0) : 0;
            QString titleString = item->name();
            if ( item->version().length() > 0 )
                    titleString += " v." + item->version();

            // perform internal scripting operations over the element
            executeScript( "document.getElementById('" + idString + "').style.color='red'" );
            executeScript( "document.getElementById('bar" + idString + "').style.width='" +
                           QString::number( pixelProgress ) + "px'" );
            executeScript( "document.getElementById('bc" + idString + "').style.backgroundColor='" +
                           (showProgress ? "gray" : "transparent") + '\'' );
            executeScript( "document.getElementById('btn" + idString + "').value='" +
                           (item->installed() ? i18n( "Uninstall" ) : i18n( "Install" )) + '\'' );
            executeScript( "document.getElementById('title" + idString + "').onClick='" +
                           (item->installed() ? "window.location.href=\"itemload:" + idString +"\";": "")+ '\''  );
            executeScript( "document.getElementById('title" + idString + "').className='" +
                           (item->installed() ? "itemLoad" : "") + '\''   );
            executeScript( "document.getElementById('title" + idString + "').title='" +
                           (item->installed() ? i18n( "Click for Open %1", titleString ) : "" ) + '\''   );
        }

    private:
        // generate the HTML contents to be displayed by the class itself
        void buildContents()
        {
            // try to get information in current locale
            QString preferredLanguage = KGlobal::locale()->language();

            begin();
            setTheAaronnesqueStyle();
            // write the html header and contents manipulation scripts
            write( "<html><body>" );

            write( "<div style='font-size:2em; border-bottom: 1px solid blue; background-color: #808080;'>Updated</div>" );

            AvailableItem::List::iterator it = m_items.begin(), iEnd = m_items.end();
            for ( ; it != iEnd; ++it )
            {
                AvailableItem * item = *it;

                // precalc the image string
                QString imageString = item->preview( preferredLanguage ).url();
                if ( imageString.length() > 1 )
                    imageString = "<div class='leftImage'><img src='" + imageString + "' border='0'></div>";

                // precalc the title string
                QString titleString = item->name();
                if ( item->version().length() > 0 )
                    titleString += " v." + item->version();

                // precalc button's text
                QString buttonText = item->installed() ? i18n( "Uninstall" ) : i18n( "Install" );

                // precalc item's dynamic strings
                QString idString = QString::number( (unsigned long)item );
                QString clickString = "window.location.href=\"item:" + idString + "\";";
                // open Installed item
                QString openInstalledItem = item->installed() ? "window.location.href=\"itemload:" + idString + "\";" : "";
		QString openInstalledItemStyles = item->installed() ? "class = itemLoad title='"+ i18n ( "Click for Open %1", titleString )+"'" : "";


                // precalc the string for displaying stars (normal+grayed)
                int starPixels = 11 + 11 * (item->rating() / 10);
                QString starsString = "<div class='star' style='width: " + QString::number( starPixels ) + "px;'></div>";
                int grayPixels = 22 + 22 * (item->rating() / 20);
                starsString = "<div class='starbg' style='width: " + QString::number( grayPixels ) + "px;'>" + starsString + "</div>";

                // precalc the string for displaying author (parsing email)
                QString authorString = item->author();
                QString emailString = authorString.section( '(', 1, 1 );
                if ( emailString.contains( '@' ) && emailString.contains( ')' ) )
                {
                    emailString = emailString.remove( ')' ).trimmed();
                    authorString = authorString.section( '(', 0, 0 ).trimmed();
                    authorString = "<a href='mailto:" + emailString + "'>" + authorString + "</a>";
                }

                // write the HTML code for the current item
                write( QString(
                      "<table class='itemBox' id='" + idString + "'><tr>"
                         "<td class='leftColumn'>"
                            // image
                              + imageString +
                            // progress bar (initially transparent)
                            "<div class='leftProgressContainer' id='bc" + idString + "'>"
                              "<div id='bar" + idString + "' class='leftProgressBar'></div>"
                            "</div>"
                            // button
                            "<div class='leftButton'>"
                              "<input type='button' id='btn" + idString + "' onClick='" + clickString + "' value='" + buttonText + "'>"
                            "</div>"
                         "</td>"
                         "<td class='contentsColumn'>"
                            // contents header: item name/score
                            "<table class='contentsHeader' cellspacing='2' cellpadding='0'><tr>"
                              "<td> <div id='title"+ idString +"' "+ openInstalledItemStyles + "' onClick='"+ openInstalledItem + "'>"+ titleString + "</div></td>"
                              "<td align='right'>" + starsString +  "</td>"
                            "</tr></table>"
                            // contents body: item description
                            "<div class='contentsBody'>"
                              + item->summary( preferredLanguage ) +
                            "</div>"
                            // contents footer: author's name/date
                            "<div class='contentsFooter'>"
                              "<em>" + authorString + "</em>, "
                              + KGlobal::locale()->formatDate( item->releaseDate(), true ) +
                            "</div>"
                         "</td>"
                      "</tr></table>" )
                );
            }

            write( "</body></html>" );
            end();
        }

        // this is the stylesheet we use
        void setTheAaronnesqueStyle()
        {
            QString hoverColor = "#000000"; //QApplication::palette().active().highlightedText().name();
            QString hoverBackground = "#f8f8f8"; //QApplication::palette().active().highlight().name();
            QString starIconPath = KStandardDirs::locate( "data", "okular/pics/ghns_star.png" );
            QString starBgIconPath = KStandardDirs::locate( "data", "okular/pics/ghns_star_gray.png" );

            // default elements style
            QString style;
            style += "body { background-color: white; color: black; padding: 0; margin: 0; }";
            style += "table, td, th { padding: 0; margin: 0; text-align: left; }";
            style += "input { color: #000080; font-size:120%; }";

            // the main item container (custom element)
            style += ".itemBox { background-color: white; color: black; width: 100%;  border-bottom: 1px solid gray; margin: 0px 0px; }";
            style += ".itemBox:hover { background-color: " + hoverBackground + "; color: " + hoverColor + "; }";
            style += ".itemLoad:hover{ cursor: pointer;}";

            // style of the item elements (4 cells with multiple containers)
            style += ".leftColumn { width: 100px; height:100%; text-align: center; }";
            style += ".leftImage {}";
            style += ".leftButton {}";
            style += ".leftProgressContainer { width: 82px; height: 10px; background-color: transparent; }";
            style += ".leftProgressBar { left: 1px; width: 0px; top: 1px; height: 8px; background-color: red; }";
            style += ".contentsColumn { vertical-align: top; }";
            style += ".contentsHeader { width: 100%; font-size: 120%; font-weight: bold; border-bottom: 1px solid #c8c8c8; }";
            style += ".contentsBody {}";
            style += ".contentsFooter {}";
            style += ".star { width: 0px; height: 24px; background-image: url(" + starIconPath + "); background-repeat: repeat-x; }";
            style += ".starbg { width: 110px; height: 24px; background-image: url(" + starBgIconPath + "); background-repeat: repeat-x; }";
            setUserStyleSheet( style );
        }

        // handle clicks on page links/buttons
        void urlSelected( const QString & link, int, int, const QString &, KParts::URLArgs )
        {
            KUrl url( link );
            QString urlProtocol = url.protocol();
            QString urlPath = url.path();

            if ( urlProtocol == "mailto" )
            {
                // clicked over a mail address
                KToolInvocation::invokeMailer( url );
            }
            else if ( urlProtocol == "item" )
            {
                // clicked over an item
                bool ok;
                unsigned long itemPointer = urlPath.toULong( &ok );
                if ( !ok )
                {
                    kWarning() << "ItemsView: error converting item pointer." << endl;
                    return;
                }

                // I love to cast pointers
                AvailableItem * item = (AvailableItem *)itemPointer;
                if ( !m_items.contains( item ) )
                {
                    kWarning() << "ItemsView: error retrieving item pointer." << endl;
                    return;
                }

                // install/uninstall the item
                if ( item->installed() )
                    m_newStuffDialog->removeItem( item );   // synchronous
                else
                    m_newStuffDialog->installItem( item );  // asynchronous
            } else if ( urlProtocol == "itemload" )
              {
                bool ok;
                unsigned long itemPointer = urlPath.toULong( &ok );
                if ( !ok )
                {
                    kWarning() << "ItemsView: error converting item pointer." << endl;
                    return;
                }
                AvailableItem * item = (AvailableItem *)itemPointer;
                if ( !m_items.contains( item ) )
                {
                    kWarning() << "ItemsView: error retrieving item pointer." << endl;
                    return;
                }
		if ( item->installed () )
		     m_newStuffDialog->loadItem( item->destinationPath() ); 
		return;

              }
        }

        // delete all the classes we own
        void clear()
        {
            // delete all items and empty local list
            AvailableItem::List::iterator it = m_items.begin(), iEnd = m_items.end();
            for ( ; it != iEnd; ++it )
                delete *it;
            m_items.clear();
        }

    private:
        NewStuffDialog * m_newStuffDialog;
        AvailableItem::List m_items;
        int m_sorting;
};


/** CORE/GUI: The main dialog that performs GHNS low level operations **/
/* internal storage structures to be used by NewStuffDialog */
struct ProvidersListJobInfo // -> Provider(s)
{
    KJob * job;
    QString receivedData;
};
struct ProviderJobInfo      // -> AvailableItem(s)
{
    QString receivedData;
};
struct ItemTransferInfo          // -> download files
{
    AvailableItem * item;
};
struct NewStuffDialogPrivate
{
    // Jobs (1 for fetching the list, unlimited for providers / items)
    ProvidersListJobInfo providersListJob;
    QMap< KJob *, ProviderJobInfo > providerJobs;
    QMap< KJob *, ItemTransferInfo > transferJobs;

    // Contents
    QList< Provider * > providers;

    // gui related vars
    QWidget * parentWidget;
    QLineEdit * searchLine;
    QComboBox * typeCombo;
    QComboBox * sortCombo;
    ItemsView * itemsView;
    QLabel * messageLabel;

    // other classes
    QTimer * messageTimer;
    QTimer * networkTimer;
};

NewStuffDialog::NewStuffDialog( QWidget * parentWidget )
    : QDialog( parentWidget ), d( new NewStuffDialogPrivate )
{
    setObjectName( "okularNewStuff" );
    setWindowTitle ( i18n ( "Get Books From Internet" ) );
    // initialize the private classes
    d->providersListJob.job = 0;

    d->parentWidget = parentWidget;

    d->messageTimer = new QTimer( this );
    d->messageTimer->setSingleShot( true );
    connect( d->messageTimer, SIGNAL( timeout() ),
             this, SLOT( slotResetMessageColors() ) );

    d->networkTimer = new QTimer( this );
    d->networkTimer->setSingleShot( true );
    connect( d->networkTimer, SIGNAL( timeout() ),
             this, SLOT( slotNetworkTimeout() ) );

    // popuplate dialog with stuff
    QBoxLayout * horLay = new QHBoxLayout( this );
    horLay->setMargin( 11 );

    // create left picture widget (if picture found)
    QPixmap p( KStandardDirs::locate( "data", "okular/pics/ghns.png" ) );
    if ( !p.isNull() )
       horLay->addWidget( new ExtendImageWidget( p, this ) );

    // create right 'main' widget
    KVBox * rightLayouter = new KVBox( this );
    rightLayouter->setSpacing( 6 );
    horLay->addWidget( rightLayouter );

      // create upper label
      QLabel * mainLabel = new QLabel( i18n("All you ever wanted, in one click!"), rightLayouter);
      QFont mainFont = mainLabel->font();
      mainFont.setBold( true );
      mainLabel->setFont( mainFont );

      // create the control panel
      QFrame * panelFrame = new QFrame( rightLayouter );
      panelFrame->setFrameStyle( QFrame::StyledPanel | QFrame::Raised );
      QGridLayout * panelLayout = new QGridLayout( panelFrame );
      panelLayout->setMargin( 11 );
      panelLayout->setSpacing( 6 );
        // add widgets to the control panel
        QLabel * label1 = new QLabel( i18n("Show:"), panelFrame );
        panelLayout->addWidget( label1, 0, 0 );
        d->typeCombo = new QComboBox( panelFrame );
        d->typeCombo->setEditable( false );
        panelLayout->addWidget( d->typeCombo, 0, 1 );
        d->typeCombo->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Minimum );
        d->typeCombo->setMinimumWidth( 150 );
        d->typeCombo->setEnabled( false );
        connect( d->typeCombo, SIGNAL( activated(int) ),
                 this, SLOT( slotLoadProvider(int) ) );

        QLabel * label2 = new QLabel( i18n("Order by:"), panelFrame );
        panelLayout->addWidget( label2, 0, 2 );
        d->sortCombo = new QComboBox( panelFrame );
        d->sortCombo->setEditable( false );
        panelLayout->addWidget( d->sortCombo, 0, 3 );
        d->sortCombo->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Minimum );
        d->sortCombo->setMinimumWidth( 100 );
        d->sortCombo->setEnabled( false );
        d->sortCombo->addItem( SmallIconSet( "fonts" ), i18n("Name") );
        d->sortCombo->addItem( SmallIconSet( "knewstuff" ), i18n("Rating") );
        d->sortCombo->addItem( SmallIconSet( "favorites" ), i18n("Downloads") );
        connect( d->sortCombo, SIGNAL( activated(int) ),
                 this, SLOT( slotSortingSelected(int) ) );

        QLabel * label3 = new QLabel( i18n("Search:"), panelFrame );
        panelLayout->addWidget( label3, 1, 0 );
        d->searchLine = new QLineEdit( panelFrame );
        panelLayout->addWidget( d->searchLine, 1, 1, 1, 3 );
        d->searchLine->setEnabled( false );

      // create the ItemsView used to display available items
      d->itemsView = new ItemsView( this, rightLayouter );

      // create bottom buttons
      KHBox * bottomLine = new KHBox( rightLayouter );
        // create info label
        d->messageLabel = new QLabel( bottomLine );
        d->messageLabel->setFrameStyle( QFrame::StyledPanel | QFrame::Raised );
        d->messageLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
        d->messageLabel->setAutoFillBackground( true );
        // close button
        KPushButton * closeButton = new KPushButton( bottomLine );
        closeButton->setGuiItem( KStdGuiItem::close() );
        //closeButton->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum )
        connect( closeButton, SIGNAL( clicked() ), this, SLOT( accept() ) );

    // start with a nice size
    resize( 700, 400 );
    slotResetMessageColors();

    // start loading providers list
    QTimer::singleShot( 100, this, SLOT( slotLoadProvidersList() ) );
}

NewStuffDialog::~NewStuffDialog()
{
    // cancel pending KJob(s) (don't like to leave orphaned threads alone)
    if ( d->providersListJob.job )
        d->providersListJob.job->kill();

    QMap< KJob *, ProviderJobInfo >::iterator pIt = d->providerJobs.begin(), pEnd = d->providerJobs.end();
    for ( ; pIt != pEnd; ++pIt )
        pIt.key()->kill();

    QMap< KJob *, ItemTransferInfo >::iterator tIt = d->transferJobs.begin(), tEnd = d->transferJobs.end();
    for ( ; tIt != tEnd; ++tIt )
        tIt.key()->kill();

    // delete all Provider descriptors
    QList< Provider * >::iterator it = d->providers.begin(), iEnd = d->providers.end();
    for ( ; it != iEnd; ++it )
        delete *it;
    d->providers.clear();

    // delete the private storage structure
    delete d;
}

void NewStuffDialog::displayMessage( const QString & msg, MessageType type, int timeOutMs )
{
    // stop the pending timer if present
    if ( d->messageTimer )
        d->messageTimer->stop();

    // set background color based on message type
    switch ( type )
    {
        case Info:
        {
            QPalette pal = d->messageLabel->palette();
            pal.setColor( QPalette::Window, pal.color( QPalette::Highlight ) );
            pal.setColor( QPalette::WindowText, pal.color( QPalette::HighlightedText ) );
            d->messageLabel->setPalette( pal );
            break;
        }
        case Error:
        {
            QPalette pal = d->messageLabel->palette();
            pal.setColor( QPalette::Window, Qt::red );
            pal.setColor( QPalette::WindowText, Qt::white );
            d->messageLabel->setPalette( pal );
            break;
        }
        default:
            slotResetMessageColors();
            break;
    }

    // set text to messageLabel
    d->messageLabel->setText( msg );

    // single shot the resetColors timer (and create it if null)
    d->messageTimer->start( timeOutMs );
}

void NewStuffDialog::installItem( AvailableItem * item )
{
    // safety check
    if ( item->url().isEmpty() || item->destinationPath().isEmpty() )
    {
        displayMessage( i18n("I do not know how to install this. Sorry, my fault."), Info );
        return;
    }

    //TODO check for AvailableItem deletion! (avoid broken pointers) -> cancel old jobs
    slotDownloadItem( item );
}

void NewStuffDialog::removeItem( AvailableItem * item )
{
    // safety check
    if ( item->destinationPath().isEmpty() )
    {
        displayMessage( i18n("I do not know how to uninstall this. Sorry, my fault."), Info );
        return;
    }

    // delete file and update item state
    KIO::NetAccess::del( item->destinationPath(), 0 );
    item->setState( AvailableItem::Normal );
    d->itemsView->updateItem( item );

    // inform the user ...
    displayMessage( i18n("%1 is no more installed.", item->name() ) );

    // ... and any listening object
    emit removedFile( item->name() );
}

void NewStuffDialog::slotResetMessageColors() // SLOT
{
    QPalette pal = d->messageLabel->palette();
    QPalette qAppPal = QApplication::palette();
    pal.setColor( QPalette::Window, qAppPal.color( QPalette::Window ) );
    pal.setColor( QPalette::WindowText, qAppPal.color( QPalette::WindowText ) );
    d->messageLabel->setPalette( pal );
}

void NewStuffDialog::loadItem( const QString & fileName )
{
    done( 0 );
    emit loadItemClicked ( KUrl ( fileName ) );
}

void NewStuffDialog::slotNetworkTimeout() // SLOT
{
    displayMessage( i18n("Timeout. Check internet connection!"), Error );
}

void NewStuffDialog::slotSortingSelected( int sortType ) // SLOT
{
    d->itemsView->setSorting( sortType );
}


//BEGIN ProvidersList Loading
void NewStuffDialog::slotLoadProvidersList()
{
    // delete an eventually pending job. note that this method should be called once!
    if ( d->providersListJob.job )
        d->providersListJob.job->kill();

    // create a job that will feed providersList data
    KIO::TransferJob * job = KIO::get( KUrl( PROVIDERS_URL ), false /*refetch*/, false /*progress*/ );
    connect( job, SIGNAL( data( KIO::Job *, const QByteArray & ) ),
             this, SLOT( slotProvidersListInfoData( KIO::Job *, const QByteArray & ) ) );
    connect( job, SIGNAL( result( KJob * ) ),
             this, SLOT( slotProvidersListResult( KJob * ) ) );

    // create a job description and data holder
    d->providersListJob.job = job;
    d->providersListJob.receivedData = "";

    // start the 'network watchdog timer'
    d->networkTimer->start( 10*1000 );

    // inform the user
    displayMessage( i18n("Loading providers list...") );
}

void NewStuffDialog::slotProvidersListInfoData( KIO::Job * job, const QByteArray & data )
{
    // safety checks
    if ( job != d->providersListJob.job || data.isEmpty() )
        return;

    // append the data buffer to the 'receivedData' string
    d->providersListJob.receivedData.append( QString::fromUtf8( data ) );
}

void NewStuffDialog::slotProvidersListResult( KJob * job )
{
    // discard contents from older jobs
    if ( d->providersListJob.job != job )
        return;

    // stop network watchdog
    d->networkTimer->stop();

    // discard job pointer (will be deleted by itself)
    d->providersListJob.job = 0;

    // if there are no errors parse dom from xml data
    QDomDocument doc;
    if ( job->error() || d->providersListJob.receivedData.isEmpty() )
    {
        displayMessage( i18n("Network: Error loading the list of providers!"), Error );
        return;
    }
    else if ( d->providersListJob.receivedData.contains("404 Not Found") )
    {
        displayMessage( i18n("Network: Cannot find the list of providers!"), Error );
        return;
    }
    else if ( !doc.setContent( d->providersListJob.receivedData ) )
    {
        displayMessage( i18n("Problems reading the list of providers. Please retry later."), Info );
        return;
    }

    // clear the current list of providers
    QList< Provider * >::iterator it = d->providers.begin(), iEnd = d->providers.end();
    for ( ; it != iEnd; ++it )
        delete *it;
    d->providers.clear();
    d->typeCombo->clear();

    // fill in the list of providers
    QDomNode providerNode = doc.documentElement().firstChild();
    while ( !providerNode.isNull() )
    {
        QDomElement elem = providerNode.toElement();
        providerNode = providerNode.nextSibling();
        if ( elem.tagName() == "provider" )
            d->providers.append( new Provider( elem ) );
    }

    // inform user about providers in the list
    if ( d->providers.count() < 1 )
    {
        displayMessage( i18n("No providers available at the moment. Please retry later."), Info );
        return;
    }
    else
        displayMessage( i18n("Loaded %1 providers", d->providers.count() ) );

    // update the providers ComboBox
    for ( it = d->providers.begin(), iEnd = d->providers.end(); it != iEnd; ++it )
    {
        const Provider * provider =  *it;
        // provider icon: using local KIconLoader, not loading from remote url
        QIcon icon = DesktopIconSet( provider->icon().url(), 16 );
        QString name = provider->name();
        // insert provider in combo
        d->typeCombo->addItem( icon, name );
    }

    // automatically load the first provider
    d->typeCombo->setEnabled( true );
    d->typeCombo->setCurrentIndex( 0 );
    QTimer::singleShot( 500, this, SLOT( slotLoadProvider() ) );
}
//END ProvidersList Loading

//BEGIN Provider contents Loading
void NewStuffDialog::slotLoadProvider( int pNumber )
{
    // safety check
    if ( !d->typeCombo->isEnabled() || pNumber < 0 || pNumber >= (int)d->providers.count() )
    {
        displayMessage( i18n("Error with this provider"), Error );
        return;
    }

    // create a job that will feed provider data
    const Provider * provider = d->providers[ pNumber ];
    KIO::TransferJob * job = KIO::get( provider->downloadUrl(), false /*refetch*/, false /*progress*/ );
    connect( job, SIGNAL( data( KIO::Job *, const QByteArray & ) ),
             this, SLOT( slotProviderInfoData( KIO::Job *, const QByteArray & ) ) );
    connect( job, SIGNAL( result( KJob * ) ),
             this, SLOT( slotProviderInfoResult( KJob * ) ) );

    // create a job description and data holder
    ProviderJobInfo info;
    d->providerJobs[ job ] = info;

    // inform the user
    displayMessage( i18n( "Loading %1...", provider->name() ) );

    // start the 'network watchdog timer'
    d->networkTimer->start( 30*1000 );

    // block any possible recourring calls while we're running
    d->typeCombo->setEnabled( false );
}

void NewStuffDialog::slotProviderInfoData( KIO::Job * job, const QByteArray & data )
{
    // safety checks
    if ( data.isEmpty() || !d->providerJobs.contains( job ) )
        return;

    // append the data buffer to the 'receivedData' string
    d->providerJobs[ job ].receivedData.append( QString::fromUtf8( data ) );
}

void NewStuffDialog::slotProviderInfoResult( KJob * job )
{
    // stop network watchdog
    d->networkTimer->stop();

    // enable gui controls
    d->typeCombo->setEnabled( true );
    d->sortCombo->setEnabled( true );

    // safety check
    if ( job->error() || !d->providerJobs.contains( job ) ||
         d->providerJobs[ job ].receivedData.isEmpty() ||
         d->providerJobs[ job ].receivedData.contains("404 Not Found") )
    {
        d->providerJobs.remove( job );
        displayMessage( i18n("Error loading provider description!"), Error );
        return;
    }

    // build XML DOM from the received data
    QDomDocument doc;
    bool docOk = doc.setContent( d->providerJobs[ job ].receivedData );
    d->providerJobs.remove( job );
    if ( !docOk )
    {
        displayMessage( i18n("Problems parsing provider description."), Info );
        return;
    }

    // create AvailableItem(s) based on the knewstuff.dtd
    AvailableItem::List itemList;
    QDomNode stuffNode = doc.documentElement().firstChild();
    while ( !stuffNode.isNull() )
    {
        QDomElement elem = stuffNode.toElement();
        stuffNode = stuffNode.nextSibling();
        // WARNING: disabled the stuff type checking (use only in kate afaik)
        if ( elem.tagName() == "stuff" /*&& elem.attribute( "type", RES_TYPE ) == RES_TYPE*/ )
            itemList.append( new AvailableItem( elem ) );
    }

    // update the control widget and inform user about the current operation
    d->itemsView->setItems( itemList );
    if ( itemList.count() )
        displayMessage( i18n("There are %1 resources available.", itemList.count() ) );
    else
        displayMessage( i18n("No resources available on this provider." ) );
}
//END Provider contents loading

//BEGIN File(s) Transferring
void NewStuffDialog::slotDownloadItem( AvailableItem * item )
{
    // create a job that will download the file
    KIO::FileCopyJob * job = KIO::file_copy( item->url(), item->destinationPath(),
        -1 /*perms*/, true /*overwrite*/, false /*resume*/, false /*showProgressInfo*/ );
    connect( job, SIGNAL( infoMessage( KJob *, const QString & ) ),
             this, SLOT( slotItemMessage( KJob *, const QString & ) ) );
    connect( job, SIGNAL( percent ( KJob *, unsigned long ) ),
             this, SLOT( slotItemPercentage( KJob *, unsigned long ) ) );
    connect( job, SIGNAL( result( KJob * ) ),
             this, SLOT( slotItemResult( KJob * ) ) );

    // create a job description and data holder
    ItemTransferInfo info;
    info.item = item;
    d->transferJobs[ job ] = info;

    // update item status
    item->setState( AvailableItem::Installing );
    item->setProgress( 0.0 );
    d->itemsView->updateItem( item );

    // inform the user
    displayMessage( i18n( "Installing '%1', this could take some time ...", item->name() ) );
}

void NewStuffDialog::slotItemMessage( KJob * job, const QString & message )
{
    // safety check
    if ( !d->transferJobs.contains( job ) )
        return;

    // update item state
    AvailableItem * item = d->transferJobs[ job ].item;
    kDebug() << "Name: " << item->name() << " msg: '" << message << "'." << endl;
    d->itemsView->updateItem( item );
}

void NewStuffDialog::slotItemPercentage( KJob * job, unsigned long percent )
{
    // safety check
    if ( !d->transferJobs.contains( job ) )
        return;

    // update item state
    AvailableItem * item = d->transferJobs[ job ].item;
    item->setProgress( (float)percent / 100.0 );
    d->itemsView->updateItem( item );
}

void NewStuffDialog::slotItemResult( KJob * job )
{
    // safety check
    if ( !d->transferJobs.contains( job ) )
        return;

    // get item and remove job
    AvailableItem * item = d->transferJobs[ job ].item;
    d->transferJobs.remove( job );

    // error handling
    if ( job->error() )
    {
        displayMessage( i18n( "Network error while retrieving %1. Installation canceled.", item->name() ), Error );
        return;
    }

    // update item state
    item->setState( AvailableItem::Normal );
    item->setProgress( 100.0 );
    d->itemsView->updateItem( item );

    /* UNCOMPRESS (specify uncompression method)
    KTar tar( fileName, "application/x-gzip" );
    tar.open( QIODevice::ReadOnly );
    const KArchiveDirectory *dir = tar.directory();
    dir->copyTo( "somedir" );
    tar.close();
    QFile::remove( fileName );
    */

    /* EXECUTE (specify the 'cmd' command)
    QStringList list = QStringList::split( " ", cmd ),
                list2;
    for ( QStringList::iterator it = list.begin(); it != list.end(); it++ )
        list2 << (*it).replace("%f", fileName);
    KProcess proc;
    proc << list2;
    proc.start( KProcess::Block );
    */

    // inform the user ...
    displayMessage( i18n( "Installed! %1 is yours now.", item->name() ), Info );

    // ... and any listening object
    emit installedFile( item->name(), item->category() );
}
//END File(s) Transferring

#include "newstuff.moc"
