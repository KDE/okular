/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "dlggeneral.h"

#include <kauthorized.h>

#include <config-okular.h>

#include "ui_dlggeneralbase.h"
#include "settings.h"

DlgGeneral::DlgGeneral( QWidget * parent, Okular::EmbedMode embedMode )
    : QWidget( parent )
{
    m_dlg = new Ui_DlgGeneralBase();
    m_dlg->setupUi( this );

    setCustomBackgroundColorButton( Okular::Settings::useCustomBackgroundColor() );

    if( embedMode == Okular::ViewerWidgetMode )
    {
        m_dlg->kcfg_SyncThumbnailsViewport->setVisible( false );
        m_dlg->kcfg_DisplayDocumentTitle->setVisible( false );
        m_dlg->kcfg_WatchFile->setVisible( false );
        m_dlg->kcfg_rtlReadingDirection->setVisible(false);
    }
    m_dlg->kcfg_ShellOpenFileInTabs->setVisible( embedMode == Okular::NativeShellMode );
}

DlgGeneral::~DlgGeneral()
{
    delete m_dlg;
}

void DlgGeneral::showEvent( QShowEvent * )
{
#if OKULAR_FORCE_DRM
    m_dlg->kcfg_ObeyDRM->hide();
#else
    if ( KAuthorized::authorize( QStringLiteral("skip_drm") ) )
        m_dlg->kcfg_ObeyDRM->show();
    else
        m_dlg->kcfg_ObeyDRM->hide();
#endif
}

void DlgGeneral::setCustomBackgroundColorButton( bool value )
{
    m_dlg->kcfg_BackgroundColor->setEnabled( value );
}
