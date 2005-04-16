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
#include <qhbox.h>
#include <qvbox.h>
#include <qpixmap.h>
#include <qfont.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qmemarray.h>
#include <qrect.h>
#include <qpainter.h>
#include <qscrollview.h>
#include <qapplication.h>
#include <kapplication.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <khtml_part.h>
#include <kio/job.h>
#include <knewstuff/provider.h>
#include <knewstuff/entry.h>

// local includes
#include "newstuff.h"

// define the providers.xml location
#define PROVIDERS_URL "http://kpdf.kde.org/newstuff/providers.xml"
// define the type of the stuff returned by providers (unused)
#define RES_TYPE "kpdf/stuff"

/** GUI: Internal Widget that displays a ver-stretchable (only) image */
class ExtendImageWidget : public QWidget
{
    public:
        ExtendImageWidget( const QPixmap & pix, QWidget * parent )
            : QWidget( parent, "", WNoAutoErase ), m_pixmap( pix )
        {
            // adjust size hint
            setFixedWidth( pix.width() );
            setMinimumHeight( pix.height() );
            // paint all own pixels
            setBackgroundMode( Qt::NoBackground );
            // create the tile image from last line of pixmap
            m_tile.resize( pix.width(), 1 );
            copyBlt( &m_tile, 0,0, &pix, 0,pix.height()-2, pix.width(),1 );
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
            QMemArray<QRect> rects = paintRegion.subtract( pixmapRect ).rects();
            for ( unsigned int i = 0; i < rects.count(); i++ )
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
        typedef QValueList< AvailableItem * > List;
        AvailableItem( const QDomElement & element, const QString & providerName )
            : Entry( element )
        {
            QString remoteUrl = payload().url();
            QString fileName = remoteUrl.section( '/', -1, -1 );
            QString extension = fileName.lower().section( '.', -1, -1 );
            // place books on the desktop
            if ( providerName.lower().contains( "book" ) )
                m_destinationFile = KGlobalSettings::desktopPath() + "/" + fileName;
            // place kpdf data on the local share/apps/kpdf/stuff
            else if ( providerName.lower().contains( "kpdf" ) )
                m_destinationFile = locateLocal( "data", "kpdf/stuff/" + fileName );
            // warn about unrecognized provider
            else
                kdDebug() << "NewStuffDialog: AvailableItem: unrecognized provider name." << endl;
        }

        // returns local destination path for the item
        const QString & destinationPath()
        {
            return m_destinationFile;
        }

        // checks if the item is already installed
        bool installed()
        {
            return QFile::exists( m_destinationFile );
        }

    private:
        QString m_destinationFile;
};


/** GUI/CORE: HTML Widget to operate on AvailableItem::List */
class ItemsView : public KHTMLPart
{
    public:
        ItemsView( QWidget * parent )
            : KHTMLPart( parent, "newStuffHTMLView" ), m_sorting( 0 )
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

    private:
        // generate the HTML contents to be displayed by the class itself
        void buildContents()
        {
            // try to get informations in current locale
            QString preferredLanguage = KGlobal::locale()->language();

            // write the html code to be rendered
            begin();
            setTheAaronnesqueStyle();
            write( "<html><body>" );

            AvailableItem::List::iterator it = m_items.begin(), iEnd = m_items.end();
            for ( ; it != iEnd; ++it )
            {
                AvailableItem * item = *it;

                // precalc the image string
                QString imageString = item->preview( preferredLanguage ).url();
                if ( imageString.length() > 1 )
                    imageString = "<div class='leftImage'><img src='" + imageString + "' border='0'></div><br />";

                // precalc the title string
                QString titleString = item->name();
                if ( item->version().length() > 0 )
                    titleString += " v." + item->version();

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
                    emailString = emailString.remove( ')' ).stripWhiteSpace();
                    authorString = authorString.section( '(', 0, 0 ).stripWhiteSpace();
                    authorString = "<a href='mailto:" + emailString + "'>" + authorString + "</a>";
                }

                // write the HTML code for the current item
                write( QString(
                      "<table class='itemBox'><tr>"
                         "<td class='leftColumn'>"
                            // image
                              + imageString +
                            // button
                            "<div class='leftButton'>"
                              "<input type='button' onClick='window.location.href=\"item:"
                              + QString::number( (unsigned long)item ) + "\";' value='" +
                              i18n( "Install" ) + "'>"
                            "</div>"
                         "</td>"
                         "<td class='contentsColumn'>"
                            // contents header: item name/score
                            "<table class='contentsHeader' cellspacing='2' cellpadding='0'><tr>"
                              "<td>" + titleString + "</td>"
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
            QString starIconPath = locate( "data", "kpdf/pics/ghns_star.png" );
            QString starBgIconPath = locate( "data", "kpdf/pics/ghns_star_gray.png" );

            // default elements style
            QString style;
            style += "body { background-color: white; color: black; padding: 0; margin: 0; }";
            style += "table, td, th { padding: 0; margin: 0; text-align: left; }";
            style += "input { color: #000080; font-size:120%; }";

            // the main item container (custom element)
            style += ".itemBox { background-color: white; color: black; width: 100%;  border-bottom: 1px solid gray; margin: 0px 0px; }";
            style += ".itemBox:hover { background-color: " + hoverBackground + "; color: " + hoverColor + "; }";

            // style of the item elements (4 cells with multiple containers)
            style += ".leftColumn { width: 100px; height:100%; text-align: center; }";
            style += ".leftImage {}";
            style += ".leftButton {}";
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
            KURL url( link );
            QString urlProtocol = url.protocol();
            QString urlPath = url.path();

            if ( urlProtocol == "mailto" )
            {
                // clicked over a mail address
                kapp->invokeMailer( url );
            }
            else if ( urlProtocol == "item" )
            {
                // clicked over an item
                bool ok;
                unsigned long itemPointer = urlPath.toULong( &ok );
                if ( !ok )
                {
                    kdWarning() << "ItemsView: error converting item pointer." << endl;
                    return;
                }
                // I love to cast pointers
                AvailableItem * item = (AvailableItem *)itemPointer;
                if ( !m_items.contains( item ) )
                {
                    kdWarning() << "ItemsView: error retrieving item pointer." << endl;
                    return;
                }
                // perform operations on the AvailableItem (maybe already installed)
                bool itemInstalled = item->installed();
                kdDebug() << itemInstalled << endl;
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
        AvailableItem::List m_items;
        int m_sorting;
};


/** CORE/GUI: The main dialog that performs GHNS low level operations **/
/* internal storage structures to be used by NewStuffDialog */
struct ProviderJobInfo
{
    const Provider * provider;
    QString receivedData;
};
struct NewStuffDialogPrivate
{
    // network classes for providers/items/files
    ProviderLoader * providerLoader;
    Provider::List * providersList;
    QMap< KIO::Job *, ProviderJobInfo > providerJobs;
    QMap< KIO::Job *, QString > transferJobs;

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
    : QDialog( parentWidget, "kpdfNewStuff" ), d( new NewStuffDialogPrivate )
{
    // initialize the private classes
    d->providerLoader = 0;
    d->providersList = 0;
    d->parentWidget = parentWidget;

    d->messageTimer = new QTimer( this );
    connect( d->messageTimer, SIGNAL( timeout() ),
             this, SLOT( slotResetMessageColors() ) );

    d->networkTimer = new QTimer( this );
    connect( d->networkTimer, SIGNAL( timeout() ),
             this, SLOT( slotNetworkTimeout() ) );

    // popuplate dialog with stuff
    QBoxLayout * horLay = new QHBoxLayout( this, 11 );

    // create left picture widget (if picture found)
    QPixmap p( locate( "data", "kpdf/pics/ghns.png" ) );
    if ( !p.isNull() )
       horLay->addWidget( new ExtendImageWidget( p, this ) );

    // create right 'main' widget
    QVBox * rightLayouter = new QVBox( this );
    rightLayouter->setSpacing( 6 );
    horLay->addWidget( rightLayouter );

      // create upper label
      QLabel * mainLabel = new QLabel( i18n("All you ever wanted, in a click!"), rightLayouter);
      QFont mainFont = mainLabel->font();
      mainFont.setBold( true );
      mainLabel->setFont( mainFont );

      // create the control panel
      QFrame * panelFrame = new QFrame( rightLayouter );
      panelFrame->setFrameStyle( QFrame::StyledPanel | QFrame::Raised );
      QGridLayout * panelLayout = new QGridLayout( panelFrame, 2, 4, 11, 6 );
        // add widgets to the control panel
        QLabel * label1 = new QLabel( i18n("Show:"), panelFrame );
        panelLayout->addWidget( label1, 0, 0 );
        d->typeCombo = new QComboBox( false, panelFrame );
        panelLayout->addWidget( d->typeCombo, 0, 1 );
        d->typeCombo->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Minimum );
        d->typeCombo->setMinimumWidth( 150 );
        d->typeCombo->setEnabled( false );
        connect( d->typeCombo, SIGNAL( activated(int) ),
                 this, SLOT( slotLoadProvider(int) ) );

        QLabel * label2 = new QLabel( i18n("Order by:"), panelFrame );
        panelLayout->addWidget( label2, 0, 2 );
        d->sortCombo = new QComboBox( false, panelFrame );
        panelLayout->addWidget( d->sortCombo, 0, 3 );
        d->sortCombo->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Minimum );
        d->sortCombo->setMinimumWidth( 100 );
        d->sortCombo->setEnabled( false );
        connect( d->sortCombo, SIGNAL( activated(int) ),
                 this, SLOT( slotSortingSelected(int) ) );

        QLabel * label3 = new QLabel( i18n("Search:"), panelFrame );
        panelLayout->addWidget( label3, 1, 0 );
        d->searchLine = new QLineEdit( panelFrame );
        panelLayout->addMultiCellWidget( d->searchLine, 1, 1, 1, 3 );
        d->searchLine->setEnabled( false );

      // create the ItemsView used to display available items
      d->itemsView = new ItemsView( rightLayouter );

      // create bottom buttons
      QHBox * bottomLine = new QHBox( rightLayouter );
        // create info label
        d->messageLabel = new QLabel( bottomLine );
        d->messageLabel->setFrameStyle( QFrame::StyledPanel | QFrame::Raised );
        d->messageLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
        // close button
        QPushButton * closeButton = new QPushButton( i18n("Close"), bottomLine );
        //closeButton->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum )
        connect( closeButton, SIGNAL( clicked() ), this, SLOT( accept() ) );

    // start with a nice size
    resize( 700, 400 );
    slotResetMessageColors();

    // start loading providers list
    QTimer::singleShot( 100, this, SLOT( slotLoadProviders() ) );
}

NewStuffDialog::~NewStuffDialog()
{
    delete d->providerLoader;
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
            d->messageLabel->setPaletteForegroundColor( palette().active().highlightedText() );
            d->messageLabel->setPaletteBackgroundColor( palette().active().highlight() );
            break;
        case Error:
            d->messageLabel->setPaletteForegroundColor( Qt::white );
            d->messageLabel->setPaletteBackgroundColor( Qt::red );
            break;
        default:
            slotResetMessageColors();
            break;
    }

    // set text to messageLabel
    d->messageLabel->setText( msg );

    // single shot the resetColors timer (and create it if null)
    d->messageTimer->start( timeOutMs, true );
}


void NewStuffDialog::slotResetMessageColors() // SLOT
{
    d->messageLabel->setPaletteForegroundColor( palette().active().text() );
    d->messageLabel->setPaletteBackgroundColor( palette().active().background() );
}

void NewStuffDialog::slotNetworkTimeout() // SLOT
{
    displayMessage( i18n("Timeout. Check internet connection!"), Error );
}

void NewStuffDialog::slotSortingSelected( int sortType ) // SLOT
{
    d->itemsView->setSorting( sortType );
}


//BEGIN Providers Loading
void NewStuffDialog::slotLoadProviders()
{
    // create the helpful loader class (may be done internally in future)
    if ( !d->providerLoader )
    {
        d->providerLoader = new ProviderLoader( d->parentWidget );
        connect( d->providerLoader, SIGNAL( providersLoaded( Provider::List* ) ),
            this, SLOT( slotProvidersLoaded( Provider::List* ) ) );
    }

    // start loading
    d->providersList = 0;
    d->providerLoader->load( RES_TYPE, PROVIDERS_URL );

    // start the 'network watchdog timer'
    d->networkTimer->start( 20*1000, true /*single shot*/ );

    // inform the user
    displayMessage( i18n("Loading providers list...") );
}

void NewStuffDialog::slotProvidersLoaded( Provider::List * list )
{
    // stop network watchdog
    d->networkTimer->stop();

    // safety check
    d->providersList = list;
    if ( !list || !list->count() )
    {
        displayMessage( i18n("Problems loading providers. Please retry later."), Error );
        return;
    }

    // update and PopUp the providers ComboBox
    d->typeCombo->clear();
    int providersNumber = list->count();
    for ( int i = 0; i < providersNumber; i++ )
    {
        const Provider * provider = list->at( i );
        // provider icon: using local KIconLoader, not loading from remote url
        QPixmap icon = DesktopIcon( provider->icon().url(), 16 );
        QString name = provider->name();
        // insert provider in combo
        d->typeCombo->insertItem( icon, name );
    }

    // inform the user
    displayMessage( i18n("Loaded %1 providers").arg( providersNumber ), Info );

    // automatically load the first provider
    d->typeCombo->setEnabled( true );
    d->typeCombo->setCurrentItem( 0 );
    QTimer::singleShot( 500, this, SLOT( slotLoadProvider() ) );
}
//END Providers Loading

//BEGIN AvailableItem(s) Loading
void NewStuffDialog::slotLoadProvider( int pNumber )
{
    // safety check
    if ( !d->providersList || !d->typeCombo->isEnabled() ||
         pNumber < 0 || pNumber >= (int)d->providersList->count() )
    {
        displayMessage( i18n("Error with this provider"), Error );
        return;
    }

    // create a job that will feed provider data
    const Provider * provider = d->providersList->at( pNumber );
    KIO::TransferJob * job = KIO::get( provider->downloadUrl(), false /*refetch*/, false /*progress*/ );
    connect( job, SIGNAL( data( KIO::Job *, const QByteArray & ) ),
             this, SLOT( slotProviderInfoData( KIO::Job *, const QByteArray & ) ) );
    connect( job, SIGNAL( result( KIO::Job * ) ),
             this, SLOT( slotProviderInfoResult( KIO::Job * ) ) );

    // create a job description and data holder
    ProviderJobInfo info;
    info.provider = provider;
    d->providerJobs[ job ] = info;

    // inform the user
    displayMessage( i18n("Loading %1...").arg( provider->name() ) );

    // start the 'network watchdog timer'
    d->networkTimer->start( 30*1000, true /*single shot*/ );

    // block any possible recourring calls while we're running
    d->typeCombo->setEnabled( false );
}

void NewStuffDialog::slotProviderInfoData( KIO::Job * job, const QByteArray & data )
{
    // safety check
    if ( data.isEmpty() || !d->providerJobs.contains( job ) )
        return;

    // append the data buffer to the 'receivedData' string
    QCString str( data, data.size() + 1 );
    d->providerJobs[ job ].receivedData.append( QString::fromUtf8( str ) );
}

void NewStuffDialog::slotProviderInfoResult( KIO::Job * job )
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
        displayMessage( i18n("Error loading provider description!"), Error );
        return;
    }

    // build XML DOM from the received data
    QDomDocument doc;
    bool docOk = doc.setContent( d->providerJobs[ job ].receivedData );
    QString providerName = d->providerJobs[ job ].provider->name();
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
            itemList.append( new AvailableItem( elem, providerName ) );
    }

    // update the control widget and inform user about the current operation
    d->itemsView->setItems( itemList );
    if ( itemList.count() )
        displayMessage( i18n("You have %1 resources available.").arg( itemList.count() ) );
    else
        displayMessage( i18n("No resources available on this provider." ) );
}
//END AvailableItem(s) loading

/* UNCOMPRESS (specify uncompression method)
KTar tar( fileName, "application/x-gzip" );
tar.open( IO_ReadOnly );
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

/* callbacks
emit something();
*/

/*
QString target = item ? item->fullName() : "/";
QString path = "/tmp"; //locateLocal( resourceString.utf8(), target)
QString file = path + "/" + target;

if ( KStandardDirs::exists( file ) )
    if ( KMessageBox::questionYesNo( d->parentWidget,
            i18n("The file '%1' already exists. Do you want to override it?")
            .arg( file ),
            QString::null, i18n("Overwrite") ) == KMessageBox::No )
        return QString();
*/

//END of the NewStuffDialog

#include "newstuff.moc"
