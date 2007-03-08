/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qlayout.h>
#include <qlabel.h>
#include <qheaderview.h>
#include <qsortfilterproxymodel.h>
#include <qtreeview.h>
#include <kicon.h>
#include <klocale.h>
#include <ksqueezedtextlabel.h>
#include <kglobalsettings.h>

// local includes
#include "propertiesdialog.h"
#include "core/document.h"

PropertiesDialog::PropertiesDialog(QWidget *parent, Okular::Document *doc)
  : KPageDialog( parent )
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

    layout->addWidget( key, row, 0, Qt::AlignRight );
    layout->addWidget( value, row, 1 );
  }

  // FONTS
  QVBoxLayout *page2Layout = 0;
  const Okular::DocumentFonts * fonts = doc->documentFonts();
  if ( fonts ) {
    // create fonts tab and layout it
    QFrame *page2 = new QFrame();
    KPageWidgetItem *item2 = addPage(page2, i18n("&Fonts"));
    item2->setIcon( KIcon( "fonts" ) );
    page2Layout = new QVBoxLayout(page2);
    page2Layout->setMargin(marginHint());
    page2Layout->setSpacing(spacingHint());
    // add a tree view
    QTreeView *view = new QTreeView(page2);
    page2Layout->addWidget(view);
    view->setRootIsDecorated(false);
    view->setAlternatingRowColors(true);
    view->header()->setClickable(true);
    view->header()->setSortIndicatorShown(true);
    // creating a proxy model so we can sort the data
    QSortFilterProxyModel *proxymodel = new QSortFilterProxyModel(view);
    FontsListModel *model = new FontsListModel(view);
    proxymodel->setSourceModel(model);
    view->setModel(proxymodel);
    // populate the klistview
    for ( QDomNode node = fonts->documentElement().firstChild(); !node.isNull(); node = node.nextSibling() ) {
      QDomElement e = node.toElement();
      model->addFont( e.attribute( "Name" ), e.attribute( "Type" ),
                      e.attribute( "Embedded" ), e.attribute( "File" ) );
    }
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
}

class LocalFontInfoStruct
{
  public:
    QString name;
    QString type;
    QString embedded;
    QString file;
};

FontsListModel::FontsListModel( QObject * parent )
  : QAbstractTableModel( parent )
{
}

FontsListModel::~FontsListModel()
{
  qDeleteAll(m_fonts);
}

void FontsListModel::addFont( const QString &name, const QString &type, const QString &embedded, const QString &file )
{
  beginInsertRows( QModelIndex(), m_fonts.size(), m_fonts.size() );

  LocalFontInfoStruct *info = new LocalFontInfoStruct();
  info->name = name;
  info->type = type;
  info->embedded = embedded;
  info->file = file;
  m_fonts << info;

  endInsertRows();
}

int FontsListModel::columnCount( const QModelIndex & ) const
{
  return 4;
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
    case 0: return m_fonts.at(index.row())->name; break;
    case 1: return m_fonts.at(index.row())->type; break;
    case 2: return m_fonts.at(index.row())->embedded; break;
    case 3: return m_fonts.at(index.row())->file; break;
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

int FontsListModel::rowCount( const QModelIndex & ) const
{
  return m_fonts.size();
}

#include "propertiesdialog.moc"
