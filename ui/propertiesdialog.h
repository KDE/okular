/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _PROPERTIESDIALOG_H_
#define _PROPERTIESDIALOG_H_

#include <kdialogbase.h>

class KPDFDocument;

class PropertiesDialog : public KDialogBase
{
  public:
  	PropertiesDialog( QWidget *parent, KPDFDocument *doc );
};

#endif
