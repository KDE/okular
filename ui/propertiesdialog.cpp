/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "propertiesdialog.h"

// qt/kde includes
#include <qlayout.h>
#include <qlabel.h>
#include <qheaderview.h>
#include <qprogressbar.h>
#include <qsortfilterproxymodel.h>
#include <qtreeview.h>
#include <qtimer.h>
#include <kicon.h>
#include <klocale.h>
#include <ksqueezedtextlabel.h>
#include <kglobalsettings.h>

// local includes
#include "core/document.h"
#include "core/fontinfo.h"

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
  QGridLayout *layout = new QGridLayout( page );
  layout->setMargin( marginHint() );
  layout->setSpacing( spacingHint() );

  // get document info, if not present display blank data and a warning
  const Okular::DocumentInfo * info = doc->documentInfo();
  if ( !info ) {
    layout->addWidget( new QLabel( i18n( "No document opened." ), page ), 0, 0 );
    return;
  }

  // mime name based on mimetype id
  QString mimeName = info->get( "mimeType" ).section( '/', -1 ).toUpper();
  setCaption( i18n( "%1 Properties", mimeName ) );

  QDomElement docElement = info->documentElement();

  int row = 0;
  int valMaxWidth = 100;
  for ( QDomNode node = docElement.firstChild(); !node.isNull(); node = node.nextSibling() ) {
    QDomElement element = node.toElement();

    QString titleString = element.attribute( "title" );
    QString valueString = element.attribute( "value" );
    if ( titleString.isEmpty() || valueString.isEmpty() )
        continue;

    // create labels and layout them
    QLabel *key = new QLabel( i18n( "%1:", titleString ), page );
    QLabel *value = new KSqueezedTextLabel( valueString, page );
    value->setTextInteractionFlags( Qt::TextSelectableByMouse );
    layout->addWidget( key, row, 0, Qt::AlignRight );
    layout->addWidget( value, row, 1 );
    row++;

    // refine maximum width of 'value' labels
    valMaxWidth = qMax( valMaxWidth, fontMetrics().width( valueString ) );
  }

  // add the number of pages if the generator hasn't done it already
  QDomNodeList list = docElement.elementsByTagName( "pages" );
  if ( list.count() == 0 ) {
    QLabel *key = new QLabel( i18n( "Pages:" ), page );
    QLabel *value = new QLabel( QString::number( doc->pages() ), page );
    value->setTextInteractionFlags( Qt::TextSelectableByMouse );

    layout->addWidget( key, row, 0, Qt::AlignRight );
    layout->addWidget( value, row, 1 );
  }

  // FONTS
  QVBoxLayout *page2Layout = 0;
  if ( doc->canProvideFontInformation() ) {
    // create fonts tab and layout it
    QFrame *page2 = new QFrame();
    m_fontPage = addPage(page2, i18n("&Fonts"));
    m_fontPage->setIcon( KIcon( "fonts" ) );
    page2Layout = new QVBoxLayout(page2);
    page2Layout->setMargin(marginHint());
    page2Layout->setSpacing(spacingHint());
    // add a tree view
    QTreeView *view = new QTreeView(page2);
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
  // TODO ?
  setMinimumSize(sizeHint());

    connect( pageWidget(), SIGNAL( currentPageChanged( KPageWidgetItem *, KPageWidgetItem * ) ),
             this, SLOT( pageChanged( KPageWidgetItem *, KPageWidgetItem * ) ) );
}

PropertiesDialog::~PropertiesDialog()
{
    m_document->stopFontReading();
}

void PropertiesDialog::pageChanged( KPageWidgetItem *current, KPageWidgetItem * )
{
    if ( current == m_fontPage && !m_fontScanStarted )
    {
        connect( m_document, SIGNAL( gotFont( const Okular::FontInfo& ) ), m_fontModel, SLOT( addFont( const Okular::FontInfo& ) ) );
        connect( m_document, SIGNAL( fontReadingProgress( int ) ), this, SLOT( slotFontReadingProgress( int ) ) );
        connect( m_document, SIGNAL( fontReadingEnded() ), this, SLOT( slotFontReadingEnded() ) );
        QTimer::singleShot( 0, this, SLOT( reallyStartFontReading() ) );

        m_fontScanStarted = true;
    }
}

void PropertiesDialog::slotFontReadingProgress( int page )
{
    m_fontProgressBar->setValue( m_fontProgressBar->maximum() * page / m_document->pages() );
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
    return parent.isValid() ? 0 : 4;
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
        case Okular::FontInfo::Unknown:
            return i18nc("Unknown font type", "Unknown");
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
  if ( !index.isValid() )
    return QVariant();

  if ( ( index.row() < 0 ) || ( index.row() >= m_fonts.size() ) )
    return QVariant();

  if ( role != Qt::DisplayRole )
    return QVariant();

  switch ( index.column() )
  {
        case 0: return m_fonts.at( index.row() ).name(); break;
        case 1: return descriptionForFontType( m_fonts.at( index.row() ).type() ); break;
        case 2: return descriptionForEmbedType( m_fonts.at( index.row() ).embedType() ); break;
        case 3: return m_fonts.at( index.row() ).file(); break;
    default:
       return QVariant();
  }
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
    case 2: return i18n( "Embedded" ); break;
    case 3: return i18n( "File" ); break;
    default:
      return QVariant();
  }
}

int FontsListModel::rowCount( const QModelIndex &parent ) const
{
    return parent.isValid() ? 0 : m_fonts.size();
}

#include "propertiesdialog.moc"
