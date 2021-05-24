/*
    SPDX-FileCopyrightText: 2006 Pino Toscano <toscano.pino@tiscali.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "dlgperformance.h"

#include <KLocalizedString>

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>

#include "settings_core.h"

DlgPerformance::DlgPerformance(QWidget *parent)
    : QWidget(parent)
    , m_memoryExplanationLabel(new QLabel(this))
{
    QFormLayout *layout = new QFormLayout(this);

    // BEGIN Checkbox: transparency effects
    QCheckBox *useTransparencyEffects = new QCheckBox(this);
    useTransparencyEffects->setText(i18nc("@option:check Config dialog, performance page", "Enable transparency effects"));
    useTransparencyEffects->setObjectName(QStringLiteral("kcfg_EnableCompositing"));
    layout->addRow(i18nc("@label Config dialog, performance page", "CPU usage:"), useTransparencyEffects);
    // END Checkbox: transparency effects

    layout->addRow(new QLabel(this));

    // BEGIN Radio buttons: memory usage
    QComboBox *m_memoryLevel = new QComboBox(this);
    m_memoryLevel->addItem(i18nc("@item:inlistbox Config dialog, performance page, memory usage", "Low"));
    m_memoryLevel->addItem(i18nc("@item:inlistbox Config dialog, performance page, memory usage", "Normal (default)"));
    m_memoryLevel->addItem(i18nc("@item:inlistbox Config dialog, performance page, memory usage", "Aggressive"));
    m_memoryLevel->addItem(i18nc("@item:inlistbox Config dialog, performance page, memory usage", "Greedy"));
    m_memoryLevel->setObjectName(QStringLiteral("kcfg_MemoryLevel"));
    layout->addRow(i18nc("@label:listbox Config dialog, performance page, memory usage", "Memory usage:"), m_memoryLevel);

    // Setup and initialize explanation label:
    m_memoryExplanationLabel->setWordWrap(true);
    layout->addRow(m_memoryExplanationLabel);
    m_memoryLevel->setCurrentIndex(0);
    slotMemoryLevelSelected(0);
    connect(m_memoryLevel, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DlgPerformance::slotMemoryLevelSelected);
    // END Radio buttons: memory usage

    layout->addRow(new QLabel(this));

    // BEGIN Checkboxes: rendering options
    QCheckBox *useTextAntialias = new QCheckBox(this);
    useTextAntialias->setText(i18nc("@option:check Config dialog, performance page", "Enable text antialias"));
    useTextAntialias->setObjectName(QStringLiteral("kcfg_TextAntialias"));
    layout->addRow(i18nc("@title:group Config dialog, performance page", "Rendering options:"), useTextAntialias);

    QCheckBox *useGraphicsAntialias = new QCheckBox(this);
    useGraphicsAntialias->setText(i18nc("@option:check Config dialog, performance page", "Enable graphics antialias"));
    useGraphicsAntialias->setObjectName(QStringLiteral("kcfg_GraphicsAntialias"));
    layout->addRow(QString(), useGraphicsAntialias);

    QCheckBox *useTextHinting = new QCheckBox(this);
    useTextHinting->setText(i18nc("@option:check Config dialog, performance page", "Enable text hinting"));
    useTextHinting->setObjectName(QStringLiteral("kcfg_TextHinting"));
    layout->addRow(QString(), useTextHinting);
    // END Checkboxes: rendering options

    //    m_dlg->cpuLabel->setPixmap(QIcon::fromTheme(QStringLiteral("cpu")).pixmap(32));
    //    m_dlg->memoryLabel->setPixmap( QIcon::fromTheme( "kcmmemory" ).pixmap(  32 ) ); // TODO: enable again when proper icon is available TODO: Figure out a new place in the layout for these pixmaps
}

void DlgPerformance::slotMemoryLevelSelected(int which)
{
    switch (which) {
    case 0:
        m_memoryExplanationLabel->setText(i18n("Keeps used memory as low as possible. Do not reuse anything. (For systems with low memory.)"));
        break;
    case 1:
        m_memoryExplanationLabel->setText(i18n("A good compromise between memory usage and speed gain. Preload next page and boost searches. (For systems with 2GB of memory, typically.)"));
        break;
    case 2:
        m_memoryExplanationLabel->setText(i18n("Keeps everything in memory. Preload next pages. Boost searches. (For systems with more than 4GB of memory.)"));
        break;
    case 3:
        // xgettext: no-c-format
        m_memoryExplanationLabel->setText(i18n("Loads and keeps everything in memory. Preload all pages. (Will use at maximum 50% of your total memory or your free memory, whatever is bigger.)"));
        break;
    }
}
