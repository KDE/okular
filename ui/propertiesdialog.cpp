/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <klocale.h>

#include <qlabel.h>

#include "document.h"
#include "properties.h"
#include "propertiesdialog.h"

propertiesDialog::propertiesDialog(QWidget *parent, KPDFDocument *doc) : KDialogBase(parent, 0, true, i18n( "Unknown file." ), Ok)
{
  // embed the properties widget (TODO switch to a dynamic generated one)
  properties *p = new properties(this);
  setMainWidget(p);
  // get document info, if not present display blank data and a warning
  const DocumentInfo * info = doc->documentInfo();
  if ( !info )
  {
    p->titleValue->setText( i18n( "No document opened!" ) );
    return;
  }
  // mime name based on mimetype id
  QString mimeName = info->mimeType.section( '/', -1 ).upper();
  setCaption( i18n("%1 properties").arg( mimeName ) );
  // fill in document property values
  p->pagesValue->setText( QString::number( doc->pages() ) );
  p->authorValue->setText( info->author );
  p->titleValue->setText( info->title );
  p->subjectValue->setText( info->subject );
  p->keywordsValue->setText( info->keywords );
  p->producerValue->setText( info->producer );
  p->creatorValue->setText( info->creator );
  p->optimizedValue->setText( info->optimization );
  p->securityValue->setText( info->encryption );
  p->versionValue->setText( info->format + " v." + info->formatVersion );
  p->createdValue->setText( info->creationDate );
  p->modifiedValue->setText( info->modificationDate );
}
