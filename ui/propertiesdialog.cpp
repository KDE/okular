/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <klocale.h>
#include <qlayout.h>
#include <qlabel.h>

// local includes
#include "propertiesdialog.h"
#include "core/document.h"

PropertiesDialog::PropertiesDialog(QWidget *parent, KPDFDocument *doc)
  : KDialogBase( Plain, i18n( "Unknown File" ), Ok, Ok, parent, 0, true, true )
{
  QWidget *page = plainPage();
  QGridLayout *layout = new QGridLayout( page, 2, 2, marginHint(), spacingHint() );

  // get document info, if not present display blank data and a warning
  const DocumentInfo * info = doc->documentInfo();
  if ( !info ) {
    layout->addWidget( new QLabel( i18n( "No document opened." ), page ), 0, 0 );
    return;
  }

  // mime name based on mimetype id
  QString mimeName = info->get( "mimeType" ).section( '/', -1 ).upper();
  setCaption( i18n("%1 Properties").arg( mimeName ) );

  QDomElement docElement = info->documentElement();

  int row = 0;
  for ( QDomNode node = docElement.firstChild(); !node.isNull(); node = node.nextSibling() ) {
    QDomElement element = node.toElement();

    if ( !element.attribute( "title" ).isEmpty() ) {
      QLabel *key = new QLabel( i18n( "%1:" ).arg( element.attribute( "title" ) ), page );
      QLabel *value = new QLabel( element.attribute( "value" ), page );

      layout->addWidget( key, row, 0, AlignRight );
      layout->addWidget( value, row, 1 );

      row++;
    }
  }

  // add the number of pages if the generator hasn't done it already
  QDomNodeList list = docElement.elementsByTagName( "pages" );
  if ( list.count() == 0 ) {
    QLabel *key = new QLabel( i18n( "Pages:" ), page );
    QLabel *value = new QLabel( QString::number( doc->pages() ), page );

    layout->addWidget( key, row, 0 );
    layout->addWidget( value, row, 1 );
  }
}
