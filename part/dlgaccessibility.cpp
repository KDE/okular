/*
    SPDX-FileCopyrightText: 2006 Pino Toscano <toscano.pino@tiscali.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "dlgaccessibility.h"

#include "settings.h"

#include <KColorButton>
#include <KLocalizedString>

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QStackedWidget>

#ifdef HAVE_SPEECH
#include <QtTextToSpeech>
#endif

DlgAccessibility::DlgAccessibility(QWidget *parent)
    : QWidget(parent)
    , m_colorModeConfigStack(new QStackedWidget(this))
{
    QFormLayout *layout = new QFormLayout(this);

    // BEGIN Checkboxes: draw border around images/links
    // ### not working yet, hide for now
    // QCheckBox *highlightImages = new QCheckBox(this);
    // highlightImages->setText(i18nc("@option:check Config dialog, accessibility page", "Draw border around images"));
    // highlightImages->setObjectName(QStringLiteral("kcfg_HighlightImages"));
    // layout->addRow(QString(), highlightImages);

    QCheckBox *highlightLinks = new QCheckBox(this);
    highlightLinks->setText(i18nc("@option:check Config dialog, accessibility page", "Draw border around links"));
    highlightLinks->setObjectName(QStringLiteral("kcfg_HighlightLinks"));
    layout->addRow(QString(), highlightLinks);
    // END Checkboxes: draw border around images/links

    layout->addRow(new QLabel(this));

    // BEGIN Change colors section
    // Checkbox: enable Change Colors feature
    QCheckBox *enableChangeColors = new QCheckBox(this);
    enableChangeColors->setText(i18nc("@option:check Config dialog, accessibility page", "Change colors"));
    enableChangeColors->setObjectName(QStringLiteral("kcfg_ChangeColors"));
    layout->addRow(QString(), enableChangeColors);

    // Label: Performance warning
    QLabel *warningLabel = new QLabel(this);
    warningLabel->setText(i18nc("@info Config dialog, accessibility page", "<b>Warning:</b> these options can badly affect drawing speed."));
    warningLabel->setWordWrap(true);
    layout->addRow(warningLabel);

    // Combobox: color modes
    QComboBox *colorMode = new QComboBox(this);
    colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Invert colors"));
    colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Change paper color"));
    colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Change dark & light colors"));
    colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Convert to black & white"));
    colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Invert lightness"));
    colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Invert luma (sRGB linear)"));
    colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Invert luma (symmetric)"));
    colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Shift hue positive"));
    colorMode->addItem(i18nc("@item:inlistbox Config dialog, accessibility page", "Shift hue negative"));
    colorMode->setObjectName(QStringLiteral("kcfg_RenderMode"));
    layout->addRow(i18nc("@label:listbox Config dialog, accessibility page", "Color mode:"), colorMode);

    m_colorModeConfigStack->setSizePolicy({QSizePolicy::Preferred, QSizePolicy::Fixed});

    // BEGIN Empty page (Only needed to hide the other pages, but it shouldn’t be huge...)
    QWidget *pageWidget = new QWidget(this);
    QFormLayout *pageLayout = new QFormLayout(pageWidget);
    m_colorModeConfigStack->addWidget(pageWidget);
    // END Empty page

    // BEGIN Change paper color page
    pageWidget = new QWidget(this);
    pageLayout = new QFormLayout(pageWidget);

    // Color button: paper color
    KColorButton *paperColor = new KColorButton(this);
    paperColor->setObjectName(QStringLiteral("kcfg_PaperColor"));
    pageLayout->addRow(i18nc("@label:chooser Config dialog, accessibility page", "Paper color:"), paperColor);

    m_colorModeConfigStack->addWidget(pageWidget);
    // END Change paper color page

    // BEGIN Change to dark & light colors page
    pageWidget = new QWidget(this);
    pageLayout = new QFormLayout(pageWidget);

    // Color button: dark color
    KColorButton *darkColor = new KColorButton(this);
    darkColor->setObjectName(QStringLiteral("kcfg_RecolorForeground"));
    pageLayout->addRow(i18nc("@label:chooser Config dialog, accessibility page", "Dark color:"), darkColor);

    // Color button: light color
    KColorButton *lightColor = new KColorButton(this);
    lightColor->setObjectName(QStringLiteral("kcfg_RecolorBackground"));
    pageLayout->addRow(i18nc("@label:chooser Config dialog, accessibility page", "Light color:"), lightColor);

    m_colorModeConfigStack->addWidget(pageWidget);
    // END Change to dark & light colors page

    // BEGIN Convert to black & white page
    pageWidget = new QWidget(this);
    pageLayout = new QFormLayout(pageWidget);

    // Slider: threshold
    QSlider *thresholdSlider = new QSlider(this);
    thresholdSlider->setMinimum(2);
    thresholdSlider->setMaximum(253);
    thresholdSlider->setOrientation(Qt::Horizontal);
    thresholdSlider->setObjectName(QStringLiteral("kcfg_BWThreshold"));
    pageLayout->addRow(i18nc("@label:slider Config dialog, accessibility page", "Threshold:"), thresholdSlider);

    // Slider: contrast
    QSlider *contrastSlider = new QSlider(this);
    contrastSlider->setMinimum(2);
    contrastSlider->setMaximum(6);
    contrastSlider->setOrientation(Qt::Horizontal);
    contrastSlider->setObjectName(QStringLiteral("kcfg_BWContrast"));
    pageLayout->addRow(i18nc("@label:slider Config dialog, accessibility page", "Contrast:"), contrastSlider);

    m_colorModeConfigStack->addWidget(pageWidget);
    // END Convert to black & white page

    layout->addRow(QString(), m_colorModeConfigStack);

    // Setup controls enabled states:
    colorMode->setCurrentIndex(0);
    slotColorModeSelected(0);
    connect(colorMode, qOverload<int>(&QComboBox::currentIndexChanged), this, &DlgAccessibility::slotColorModeSelected);

    enableChangeColors->setChecked(false);
    colorMode->setEnabled(false);
    connect(enableChangeColors, &QCheckBox::toggled, colorMode, &QComboBox::setEnabled);
    m_colorModeConfigStack->setEnabled(false);
    connect(enableChangeColors, &QCheckBox::toggled, m_colorModeConfigStack, &QWidget::setEnabled);
    // END Change colors section

#ifdef HAVE_SPEECH
    layout->addRow(new QLabel(this));

    // BEGIN Text-to-speech section
    QComboBox *ttsEngine = new QComboBox(this);
    // Populate tts engines and use their names directly as key and item text:
    const QStringList engines = QTextToSpeech::availableEngines();
    for (const QString &engine : engines) {
        ttsEngine->addItem(engine);
    }
    ttsEngine->setProperty("kcfg_property", QByteArray("currentText"));
    ttsEngine->setObjectName(QStringLiteral("kcfg_ttsEngine"));
    layout->addRow(i18nc("@label:listbox Config dialog, accessibility page", "Text-to-speech engine:"), ttsEngine);
    // END Text-to-speech section
#endif
}

void DlgAccessibility::slotColorModeSelected(int mode)
{
    if (mode == Okular::Settings::EnumRenderMode::Paper) {
        m_colorModeConfigStack->setCurrentIndex(1);
    } else if (mode == Okular::Settings::EnumRenderMode::Recolor) {
        m_colorModeConfigStack->setCurrentIndex(2);
    } else if (mode == Okular::Settings::EnumRenderMode::BlackWhite) {
        m_colorModeConfigStack->setCurrentIndex(3);
    } else {
        m_colorModeConfigStack->setCurrentIndex(0);
    }
}
