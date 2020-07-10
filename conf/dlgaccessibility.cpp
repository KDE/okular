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

#include "settings.h"

#ifdef HAVE_SPEECH
#include <QtTextToSpeech>
#endif

DlgAccessibility::DlgAccessibility(QWidget *parent)
    : QWidget(parent)
    , m_selected(0)
{
    m_dlg = new Ui_DlgAccessibilityBase();
    m_dlg->setupUi(this);

    // ### not working yet, hide for now
    m_dlg->kcfg_HighlightImages->hide();

    m_color_pages.append(m_dlg->page_invert);
    m_color_pages.append(m_dlg->page_paperColor);
    m_color_pages.append(m_dlg->page_darkLight);
    m_color_pages.append(m_dlg->page_bw);
    m_color_pages.append(m_dlg->page_invertLightness);
    m_color_pages.append(m_dlg->page_invertLuma);
    m_color_pages.append(m_dlg->page_invertLumaSymmetric);
    m_color_pages.append(m_dlg->page_hueShiftPositive);
    m_color_pages.append(m_dlg->page_hueShiftNegative);
    for (QWidget *page : qAsConst(m_color_pages)) {
        page->hide();
    }
    m_color_pages[m_selected]->show();

#ifdef HAVE_SPEECH
    // Populate tts engines
    const QStringList engines = QTextToSpeech::availableEngines();
    for (const QString &engine : engines) {
        m_dlg->kcfg_ttsEngine->addItem(engine);
    }
    m_dlg->kcfg_ttsEngine->setProperty("kcfg_property", QByteArray("currentText"));
#else
    m_dlg->speechBox->hide();
#endif

    connect(m_dlg->kcfg_RenderMode, static_cast<void (KComboBox::*)(int)>(&KComboBox::currentIndexChanged), this, &DlgAccessibility::slotColorMode);
}

DlgAccessibility::~DlgAccessibility()
{
    delete m_dlg;
}

void DlgAccessibility::slotColorMode(int mode)
{
    m_color_pages[m_selected]->hide();
    m_color_pages[mode]->show();

    m_selected = mode;
}

#include "moc_dlgaccessibility.cpp"
