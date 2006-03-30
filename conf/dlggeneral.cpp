/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <kauthorized.h>

#include <config.h>

#include "dlggeneralbase.h"

#include "dlggeneral.h"

DlgGeneral::DlgGeneral( QWidget * parent )
    : QWidget( parent )
{
    m_dlg = new Ui_DlgGeneralBase();
    m_dlg->setupUi( this );
}

void DlgGeneral::showEvent( QShowEvent * )
{
#if KPDF_FORCE_DRM
    m_dlg->kcfg_ObeyDRM->hide();
#else
    if ( KAuthorized::authorize( "skip_drm" ) )
        m_dlg->kcfg_ObeyDRM->show();
    else
        m_dlg->kcfg_ObeyDRM->hide();
#endif
}

