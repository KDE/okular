/***************************************************************************
 *   Copyright (C) 2006,2008 by Pino Toscano <pino@kde.org>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "dlgpresentation.h"

#include "ui_dlgpresentationbase.h"
#include "widgetdrawingtools.h"

#include <QApplication>
#include <QScreen>

#include "settings.h"

DlgPresentation::DlgPresentation(QWidget *parent)
    : QWidget(parent)
{
    m_dlg = new Ui_DlgPresentationBase();
    m_dlg->setupUi(this);

    WidgetDrawingTools *kcfg_DrawingTools = new WidgetDrawingTools(m_dlg->annotationToolsGroupBox);
    m_dlg->verticalLayout_4->addWidget(kcfg_DrawingTools);
    kcfg_DrawingTools->setObjectName(QStringLiteral("kcfg_DrawingTools"));

    m_dlg->kcfg_SlidesAdvanceTime->setSuffix(ki18ncp("Advance every %1 seconds", " second", " seconds"));
}

DlgPresentation::~DlgPresentation()
{
    delete m_dlg;
}

PreferredScreenSelector::PreferredScreenSelector(QWidget *parent)
    : QComboBox(parent)
    , m_disconnectedScreenNumber(k_noDisconnectedScreenNumber)
{
    // Populate list:
    static_assert(k_specialScreenCount == 2, "Special screens unknown to PreferredScreenSelector constructor.");
    addItem(i18nc("@item:inlistbox Config dialog, presentation page, preferred screen", "Current Screen"));
    addItem(i18nc("@item:inlistbox Config dialog, presentation page, preferred screen", "Default Screen"));

    const QList<QScreen *> screens = qApp->screens();
    for (int screenNumber = 0; screenNumber < screens.count(); ++screenNumber) {
        QScreen *screen = screens.at(screenNumber);
        addItem(i18nc("@item:inlistbox Config dialog, presentation page, preferred screen. %1 is the screen number (0, 1, ...). %2 is the screen manufacturer name. %3 is the screen model name. %4 is the screen name like DVI-0",
                      "Screen %1 (%2 %3 %4)",
                      screenNumber,
                      screen->manufacturer(),
                      screen->model(),
                      screen->name()));
    }

    // If a disconnected screen is configured, it will be appended last:
    m_disconnectedScreenIndex = count();

    // KConfigWidgets setup:
    setProperty("kcfg_property", QByteArray("preferredScreen"));
    connect(this, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) { emit preferredScreenChanged(index - k_specialScreenCount); });
}

void PreferredScreenSelector::setPreferredScreen(int newScreen)
{
    // Check whether the new screen is not in the list of connected screens:
    if (newScreen >= m_disconnectedScreenIndex - k_specialScreenCount) {
        if (m_disconnectedScreenNumber == k_noDisconnectedScreenNumber) {
            addItem(QString());
        }
        setItemText(m_disconnectedScreenIndex, i18nc("@item:inlistbox Config dialog, presentation page, preferred screen. %1 is the screen number (0, 1, ...), hopefully not 0.", "Screen %1 (disconnected)", newScreen));
        setCurrentIndex(m_disconnectedScreenIndex);
        m_disconnectedScreenNumber = newScreen;
        return;
    }

    setCurrentIndex(newScreen + k_specialScreenCount);

    // screenChanged() is emitted through currentIndexChanged().
}

int PreferredScreenSelector::preferredScreen() const
{
    if (currentIndex() == m_disconnectedScreenIndex) {
        return m_disconnectedScreenNumber;
    } else {
        return currentIndex() - k_specialScreenCount;
    }
}
