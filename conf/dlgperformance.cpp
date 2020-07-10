/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "dlgperformance.h"

#include <QButtonGroup>
#include <QFont>

#include <KConfigDialogManager>

#include "settings_core.h"
#include "ui_dlgperformancebase.h"

DlgPerformance::DlgPerformance(QWidget *parent)
    : QWidget(parent)
{
    m_dlg = new Ui_DlgPerformanceBase();
    m_dlg->setupUi(this);

    QFont labelFont = m_dlg->descLabel->font();
    labelFont.setBold(true);
    m_dlg->descLabel->setFont(labelFont);

    m_dlg->cpuLabel->setPixmap(QIcon::fromTheme(QStringLiteral("cpu")).pixmap(32));
    //    m_dlg->memoryLabel->setPixmap( QIcon::fromTheme( "kcmmemory" ).pixmap(  32 ) ); // TODO: enable again when proper icon is available

    m_dlg->memoryLevelGroup->setId(m_dlg->lowRadio, 0);
    m_dlg->memoryLevelGroup->setId(m_dlg->normalRadio, 1);
    m_dlg->memoryLevelGroup->setId(m_dlg->aggressiveRadio, 2);
    m_dlg->memoryLevelGroup->setId(m_dlg->greedyRadio, 3);

    connect(m_dlg->memoryLevelGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &DlgPerformance::radioGroup_changed);
}

DlgPerformance::~DlgPerformance()
{
    delete m_dlg;
}

void DlgPerformance::radioGroup_changed(int which)
{
    switch (which) {
    case 0:
        m_dlg->descLabel->setText(i18n("Keeps used memory as low as possible. Do not reuse anything. (For systems with low memory.)"));
        break;
    case 1:
        m_dlg->descLabel->setText(i18n("A good compromise between memory usage and speed gain. Preload next page and boost searches. (For systems with 2GB of memory, typically.)"));
        break;
    case 2:
        m_dlg->descLabel->setText(i18n("Keeps everything in memory. Preload next pages. Boost searches. (For systems with more than 4GB of memory.)"));
        break;
    case 3:
        // xgettext: no-c-format
        m_dlg->descLabel->setText(i18n("Loads and keeps everything in memory. Preload all pages. (Will use at maximum 50% of your total memory or your free memory, whatever is bigger.)"));
        break;
    }
}

#include "moc_dlgperformance.cpp"
