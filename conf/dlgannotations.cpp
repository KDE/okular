/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "dlgannotations.h"

#include <kconfigdialogmanager.h>

#include "widgetannottools.h"
#include "ui_dlgannotationsbase.h"

DlgAnnotations::DlgAnnotations( QWidget * parent )
    : QWidget( parent )
{
    Ui_DlgAnnotationsBase dlg;
    dlg.setupUi( this );

    WidgetAnnotTools * kcfg_AnnotationTools = new WidgetAnnotTools( dlg.annotToolsGroup );
    dlg.annotToolsPlaceholder->addWidget( kcfg_AnnotationTools );
    kcfg_AnnotationTools->setObjectName( QStringLiteral("kcfg_AnnotationTools") );

    KConfigDialogManager::changedMap()->insert( QStringLiteral("WidgetAnnotTools") , SIGNAL(changed()) );
}
