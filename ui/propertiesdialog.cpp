/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "propertiesdialog.h"

// qt/kde includes
#include <qfile.h>
#include <qlayout.h>
#include <qformlayout.h>
#include <qlabel.h>
#include <qheaderview.h>
#include <qmenu.h>
#include <qprogressbar.h>
#include <qpushbutton.h>
#include <qsortfilterproxymodel.h>
#include <qtreeview.h>
#include <qtimer.h>
#include <kfiledialog.h>
#include <kicon.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ksqueezedtextlabel.h>
#include <kglobalsettings.h>
#include <kurl.h>

// local includes
#include "core/document.h"
#include "core/fontinfo.h"

static const int IsExtractableRole = Qt::UserRole;
static const int FontInfoRole = Qt::UserRole + 1;

PropertiesDialog::PropertiesDialog(QWidget *parent, Okular::Document *doc)
    : KPageDialog( parent ), m_document( doc ), m_fontPage( 0 ),
      m_fontModel( 0 ), m_fontInfo( 0 ), m_fontProgressBar( 0 ),
      m_fontScanStarted( false )
{
  setFaceType( Tabbed );
  setCaption( i18n( "Unknown File" ) );
  setButtons( Ok );

  // PROPERTIES
  QFrame *page = new QFrame();
  KPageWidgetItem *item = addPage( page, i18n( "&Properties" ) );
  item->setIcon( KIcon( "document-properties" ) );

  // get document info, if not present display blank data and a warning
  const Okular::DocumentInfo * info = doc->documentInfo();
  if ( !info ) {
    QVBoxLayout *layout = new QVBoxLayout( page );
    layout->addWidget( new QLabel( i18n( "No document opened." ), page ) );
    return;
  }

  QFormLayout *layout = new QFormLayout( page );

  // mime name based on mimetype id
  QString mimeName = info->get( "mimeType" ).section( '/', -1 ).toUpper();
  setCaption( i18n( "%1 Properties", mimeName ) );

  QDomElement docElement = info->documentElement();

  int valMaxWidth = 100;

  /* obtains the properties list, conveniently ordered */
  QStringList orderedProperties;
  orderedProperties << Okular::DocumentInfo::getKeyString( Okular::DocumentInfo::FilePath )
                    << Okular::DocumentInfo::getKeyString( Okular::DocumentInfo::PagesSize )
                    << Okular::DocumentInfo::getKeyString( Okular::DocumentInfo::DocumentSize );
  for (Okular::DocumentInfo::Key ks = Okular::DocumentInfo::Title; 
          ks <= Okular::DocumentInfo::Keywords; 
          ks = Okular::DocumentInfo::Key( ks+1 ) ) {
      orderedProperties << Okular::DocumentInfo::getKeyString( ks );
  }
  for ( QDomNode node = docElement.firstChild(); !node.isNull(); node = node.nextSibling() ) {
    const QDomElement element = node.toElement();

    const QString titleString = element.attribute( "title" );
    const QString valueString = element.attribute( "value" );
    if ( titleString.isEmpty() || valueString.isEmpty() )
        continue;
    if ( !orderedProperties.contains( element.tagName() ) ) {
        orderedProperties << element.tagName();
    }
  }

  QDomNodeList list;
  
  for ( QStringList::Iterator it = orderedProperties.begin(); 
          it != orderedProperties.end(); ++it ) {
      list = docElement.elementsByTagName( (*it) );
      if ( list.count() == 1 ) {

        QDomElement element = list.at(0).toElement();
        const QString titleString = element.attribute( "title" );
        const QString valueString = element.attribute( "value" );
        if ( titleString.isEmpty() || valueString.isEmpty() )
            continue;

        // create labels and layout them
        QWidget *value = NULL;
        if ( element.tagName() == Okular::DocumentInfo::getKeyString( Okular::DocumentInfo::MimeType ) ) {
            /// for mime type fields, show icon as well
            value = new QWidget( page );
            /// place icon left of mime type's name
            QHBoxLayout *hboxLayout = new QHBoxLayout( value );
            hboxLayout->setMargin( 0 );
            /// retrieve icon and place it in a QLabel
            KMimeType::Ptr mimeType = KMimeType::mimeType( valueString );
            KSqueezedTextLabel *squeezed;
            if (!mimeType.isNull()) {
                /// retrieve icon and place it in a QLabel
                QLabel *pixmapLabel = new QLabel( value );
                hboxLayout->addWidget( pixmapLabel, 0 );
                pixmapLabel->setPixmap( KIconLoader::global()->loadMimeTypeIcon( mimeType->iconName(), KIconLoader::Small ) );
                /// mime type's name and label
                squeezed = new KSqueezedTextLabel( i18nc( "mimetype information, example: \"PDF Document (application/pdf)\"", "%1 (%2)", mimeType->comment(), valueString ), value );
            } else {
                /// only mime type name
                squeezed = new KSqueezedTextLabel( valueString, value );
            }
            squeezed->setTextInteractionFlags( Qt::TextSelectableByMouse );
            hboxLayout->addWidget( squeezed, 1 );
        } else {
            /// default for any other document information
            KSqueezedTextLabel *label = new KSqueezedTextLabel( valueString, page );
            label->setTextInteractionFlags( Qt::TextSelectableByMouse );
            value = label;
        }
        layout->addRow( new QLabel( i18n( "%1:", titleString ) ), value);

        // refine maximum width of 'value' labels
        valMaxWidth = qMax( valMaxWidth, fontMetrics().width( valueString ) );
    }
  }

  // FONTS
  QVBoxLayout *page2Layout = 0;
  if ( doc->canProvideFontInformation() ) {
    // create fonts tab and layout it
    QFrame *page2 = new QFrame();
    m_fontPage = addPage(page2, i18n("&Fonts"));
    m_fontPage->setIcon( KIcon( "preferences-desktop-font" ) );
    page2Layout = new QVBoxLayout(page2);
    page2Layout->setMargin(marginHint());
    page2Layout->setSpacing(spacingHint());
    // add a tree view
    QTreeView *view = new QTreeView(page2);
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showFontsMenu(QPoint)));
    page2Layout->addWidget(view);
    view->setRootIsDecorated(false);
    view->setAlternatingRowColors(true);
    view->setSortingEnabled( true );
    // creating a proxy model so we can sort the data
    QSortFilterProxyModel *proxymodel = new QSortFilterProxyModel(view);
    proxymodel->setDynamicSortFilter( true );
    proxymodel->setSortCaseSensitivity( Qt::CaseInsensitive );
    m_fontModel = new FontsListModel(view);
    proxymodel->setSourceModel(m_fontModel);
    view->setModel(proxymodel);
    view->sortByColumn( 0, Qt::AscendingOrder );
    m_fontInfo = new QLabel( this );
    page2Layout->addWidget( m_fontInfo );
    m_fontInfo->setText( i18n( "Reading font information..." ) );
    m_fontInfo->hide();
    m_fontProgressBar = new QProgressBar( this );
    page2Layout->addWidget( m_fontProgressBar );
    m_fontProgressBar->setRange( 0, 100 );
    m_fontProgressBar->setValue( 0 );
    m_fontProgressBar->hide();
  }

  // current width: left columnt + right column + dialog borders
  int width = layout->minimumSize().width() + valMaxWidth + 2 * marginHint() + spacingHint() + 30;
  if ( page2Layout )
    width = qMax( width, page2Layout->sizeHint().width() + marginHint() + spacingHint() + 31 );
  // stay inside the 2/3 of the screen width
  QRect screenContainer = KGlobalSettings::desktopGeometry( this );
  width = qMin( width, 2*screenContainer.width()/3 );
  resize(width, 1);

    connect( pageWidget(), SIGNAL(currentPageChanged(KPageWidgetItem*,KPageWidgetItem*)),
             this, SLOT(pageChanged(KPageWidgetItem*,KPageWidgetItem*)) );
}

PropertiesDialog::~PropertiesDialog()
{
    m_document->stopFontReading();
}

void PropertiesDialog::pageChanged( KPageWidgetItem *current, KPageWidgetItem * )
{
    if ( current == m_fontPage && !m_fontScanStarted )
    {
        connect( m_document, SIGNAL(gotFont(Okular::FontInfo)), m_fontModel, SLOT(addFont(Okular::FontInfo)) );
        connect( m_document, SIGNAL(fontReadingProgress(int)), this, SLOT(slotFontReadingProgress(int)) );
        connect( m_document, SIGNAL(fontReadingEnded()), this, SLOT(slotFontReadingEnded()) );
        QTimer::singleShot( 0, this, SLOT(reallyStartFontReading()) );

        m_fontScanStarted = true;
    }
}

void PropertiesDialog::slotFontReadingProgress( int page )
{
    m_fontProgressBar->setValue( m_fontProgressBar->maximum() * ( page + 1 ) / m_document->pages() );
}

void PropertiesDialog::slotFontReadingEnded()
{
    m_fontInfo->hide();
    m_fontProgressBar->hide();
}

void PropertiesDialog::reallyStartFontReading()
{
    m_fontInfo->show();
    m_fontProgressBar->show();
    m_document->startFontReading();
}

void PropertiesDialog::showFontsMenu(const QPoint &pos)
{
    QTreeView *view = static_cast<QTreeView*>(sender());
    QModelIndex index = view->indexAt(pos);
    if (index.data(IsExtractableRole).toBool())
    {
        QMenu *menu = new QMenu(this);
        menu->addAction( i18nc("@action:inmenu", "&Extract Font") );
        QAction *result = menu->exec(view->viewport()->mapToGlobal(pos));
        if (result)
        {
            Okular::FontInfo fi = index.data(FontInfoRole).value<Okular::FontInfo>();
            const QString caption = i18n( "Where do you want to save %1?", fi.name() );
            const QString path = KFileDialog::getSaveFileName( fi.name(), QString(), this, caption, KFileDialog::ConfirmOverwrite );
            if ( path.isEmpty() )
                return;
            
            QFile f( path );
            if ( f.open( QIODevice::WriteOnly ) )
            {
                f.write( m_document->fontData(fi) );
                f.close();
            }
            else
            {
                KMessageBox::error( this, i18n( "Could not open \"%1\" for writing. File was not saved.", path ) );
            }
        }
    }
}

FontsListModel::FontsListModel( QObject * parent )
  : QAbstractTableModel( parent )
{
}

FontsListModel::~FontsListModel()
{
}

void FontsListModel::addFont( const Okular::FontInfo &fi )
{
  beginInsertRows( QModelIndex(), m_fonts.size(), m_fonts.size() );

  m_fonts << fi;

  endInsertRows();
}

int FontsListModel::columnCount( const QModelIndex &parent ) const
{
    return parent.isValid() ? 0 : 3;
}

static QString descriptionForFontType( Okular::FontInfo::FontType type )
{
    switch ( type )
    {
        case Okular::FontInfo::Type1:
            return i18n("Type 1");
            break;
        case Okular::FontInfo::Type1C:
            return i18n("Type 1C");
            break;
        case Okular::FontInfo::Type1COT:
            return i18nc("OT means OpenType", "Type 1C (OT)");
            break;
        case Okular::FontInfo::Type3:
            return i18n("Type 3");
            break;
        case Okular::FontInfo::TrueType:
            return i18n("TrueType");
            break;
        case Okular::FontInfo::TrueTypeOT:
            return i18nc("OT means OpenType", "TrueType (OT)");
            break;
        case Okular::FontInfo::CIDType0:
            return i18n("CID Type 0");
            break;
        case Okular::FontInfo::CIDType0C:
            return i18n("CID Type 0C");
            break;
        case Okular::FontInfo::CIDType0COT:
            return i18nc("OT means OpenType", "CID Type 0C (OT)");
            break;
        case Okular::FontInfo::CIDTrueType:
            return i18n("CID TrueType");
            break;
        case Okular::FontInfo::CIDTrueTypeOT:
            return i18nc("OT means OpenType", "CID TrueType (OT)");
            break;
        case Okular::FontInfo::TeXPK:
            return i18n("TeX PK");
            break;
        case Okular::FontInfo::TeXVirtual:
            return i18n("TeX virtual");
            break;
        case Okular::FontInfo::TeXFontMetric:
            return i18n("TeX Font Metric");
            break;
        case Okular::FontInfo::TeXFreeTypeHandled:
            return i18n("TeX FreeType-handled");
            break;
        case Okular::FontInfo::Unknown:
            return i18nc("Unknown font type", "Unknown");
            break;
     }
     return QString();
}

static QString pathOrDescription( const Okular::FontInfo &font )
{
    switch ( font.embedType() )
    {
        case Okular::FontInfo::NotEmbedded:
            return font.file();
            break;
        case Okular::FontInfo::EmbeddedSubset:
            return i18n("Embedded (subset)");
            break;
        case Okular::FontInfo::FullyEmbedded:
            return i18n("Fully embedded");
            break;
     }
     return QString();
}

static QString descriptionForEmbedType( Okular::FontInfo::EmbedType type )
{
    switch ( type )
    {
        case Okular::FontInfo::NotEmbedded:
            return i18n("No");
            break;
        case Okular::FontInfo::EmbeddedSubset:
            return i18n("Yes (subset)");
            break;
        case Okular::FontInfo::FullyEmbedded:
            return i18n("Yes");
            break;
     }
     return QString();
}

QVariant FontsListModel::data( const QModelIndex &index, int role ) const
{
    if ( !index.isValid() || index.row() < 0 || index.row() >= m_fonts.count() )
        return QVariant();

    switch ( role )
    {
        case Qt::DisplayRole:
            switch ( index.column() )
            {
                case 0:
                {
                    QString fontname = m_fonts.at( index.row() ).name();
                    return fontname.isEmpty() ? i18nc( "font name not available (empty)", "[n/a]" ) : fontname;
                    break;
                }
                case 1:
                    return descriptionForFontType( m_fonts.at( index.row() ).type() );
                    break;
                case 2:
                    return pathOrDescription( m_fonts.at( index.row() ) );
                    break;
            }
            break;
        case Qt::ToolTipRole:
        {
            QString fontname = m_fonts.at( index.row() ).name();
            if ( fontname.isEmpty() )
                fontname = i18n( "Unknown font" );
            QString tooltip = QString::fromLatin1( "<html><b>" ) +  fontname + QString::fromLatin1( "</b>" );
            if ( m_fonts.at( index.row() ).embedType() == Okular::FontInfo::NotEmbedded )
                tooltip += QString::fromLatin1( " (<span style=\"font-family: '%1'\">%2</span>)" ).arg( fontname ).arg( fontname );
            tooltip += QString::fromLatin1( "<br />" ) + i18n( "Embedded: %1", descriptionForEmbedType( m_fonts.at( index.row() ).embedType() ) );
            tooltip += QString::fromLatin1( "</html>" );
            return tooltip;
            break;
        }
        case IsExtractableRole:
        {
            return m_fonts.at( index.row() ).canBeExtracted();
        }
        case FontInfoRole:
        {
            QVariant v;
            v.setValue( m_fonts.at( index.row() ) );
            return v;
        }
    }

  return QVariant();
}

QVariant FontsListModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( orientation != Qt::Horizontal )
    return QVariant();

  if ( role == Qt::TextAlignmentRole )
    return QVariant( Qt::AlignLeft );

  if ( role != Qt::DisplayRole )
    return QVariant();

  switch ( section )
  {
    case 0: return i18n( "Name" ); break;
    case 1: return i18n( "Type" ); break;
    case 2: return i18n( "File" ); break;
    default:
      return QVariant();
  }
}

int FontsListModel::rowCount( const QModelIndex &parent ) const
{
    return parent.isValid() ? 0 : m_fonts.size();
}

#include "propertiesdialog.moc"

/* kate: replace-tabs on; indent-width 4; */
