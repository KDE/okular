/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "dlgaccessibility.h"

#include "ui_dlgaccessibilitybase.h"

DlgAccessibility::DlgAccessibility( QWidget * parent )
    : QWidget( parent )
{
    Ui_DlgAccessibilityBase *dlg = new Ui_DlgAccessibilityBase();
    dlg->setupUi( this );
}
