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

propertiesDialog::propertiesDialog(QWidget *parent, KPDFDocument *doc) : KDialogBase(parent, 0, true, i18n("PDF properties"), Ok)
{
  properties *p = new properties(this);
  setMainWidget(p);
  p->pagesValue->setText(QString::number(doc->pages()));
  p->authorValue->setText(doc->author());
  p->titleValue->setText(doc->title());
  p->subjectValue->setText(doc->subject());
  p->keywordsValue->setText(doc->keywords());
  p->producerValue->setText(doc->producer());
  p->creatorValue->setText(doc->creator());
  if (doc->optimized()) p->optimizedValue->setText(i18n("Yes"));
  else p->optimizedValue->setText(i18n("No"));
  if (doc->encrypted()) p->securityValue->setText(i18n("Encrypted"));
  else p->securityValue->setText(i18n("No"));
  p->versionValue->setText(QString::number(doc->PDFversion()));
  p->createdValue->setText(doc->creationDate());
  p->modifiedValue->setText(doc->modificationDate());
}
