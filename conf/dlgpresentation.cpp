/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "dlgpresentation.h"

#include "ui_dlgpresentationbase.h"

DlgPresentation::DlgPresentation( QWidget * parent )
    : QWidget( parent )
{
    Ui_DlgPresentationBase *dlg = new Ui_DlgPresentationBase();
    dlg->setupUi( this );
}
