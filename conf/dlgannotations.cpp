/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "dlgannotations.h"

#include <KConfigDialogManager>

#include "ui_dlgannotationsbase.h"
#include "widgetannottools.h"

DlgAnnotations::DlgAnnotations(QWidget *parent)
    : QWidget(parent)
{
    Ui_DlgAnnotationsBase dlg;
    dlg.setupUi(this);

    WidgetAnnotTools *kcfg_QuickAnnotationTools = new WidgetAnnotTools(dlg.annotToolsGroup);
    dlg.annotToolsPlaceholder->addWidget(kcfg_QuickAnnotationTools);
    kcfg_QuickAnnotationTools->setObjectName(QStringLiteral("kcfg_QuickAnnotationTools"));

    KConfigDialogManager::changedMap()->insert(QStringLiteral("WidgetAnnotTools"), SIGNAL(changed()));
}
