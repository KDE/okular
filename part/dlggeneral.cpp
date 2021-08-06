/*
    SPDX-FileCopyrightText: 2006 Pino Toscano <toscano.pino@tiscali.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "dlggeneral.h"

#include <KAuthorized>
#include <KColorButton>
#include <KLocalizedString>

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>

#include <config-okular.h>

#include "settings.h"

DlgGeneral::DlgGeneral(QWidget *parent, Okular::EmbedMode embedMode)
    : QWidget(parent)
{
    QFormLayout *layout = new QFormLayout(this);

    // BEGIN Appearance section
    // Checkbox: use smooth scrolling
    QCheckBox *useSmoothScrolling = new QCheckBox(this);
    useSmoothScrolling->setText(i18nc("@option:check Config dialog, general page", "Use smooth scrolling"));
    useSmoothScrolling->setObjectName(QStringLiteral("kcfg_SmoothScrolling"));
    layout->addRow(i18nc("@title:group Config dialog, general page", "Appearance:"), useSmoothScrolling);

    // Checkbox: show scrollbars
    QCheckBox *showScrollbars = new QCheckBox(this);
    showScrollbars->setText(i18nc("@option:check Config dialog, general page", "Show scrollbars"));
    showScrollbars->setObjectName(QStringLiteral("kcfg_ShowScrollBars"));
    layout->addRow(QString(), showScrollbars);

    if (embedMode != Okular::ViewerWidgetMode) {
        // Checkbox: scroll thumbnails automatically
        QCheckBox *scrollThumbnails = new QCheckBox(this);
        scrollThumbnails->setText(i18nc("@option:check Config dialog, general page", "Link the thumbnails with the page"));
        scrollThumbnails->setObjectName(QStringLiteral("kcfg_SyncThumbnailsViewport"));
        layout->addRow(QString(), scrollThumbnails);
    }

    // Checkbox: Show welcoming messages (the balloons or OSD)
    QCheckBox *showOSD = new QCheckBox(this);
    showOSD->setText(i18nc("@option:check Config dialog, general page", "Show hints and info messages"));
    showOSD->setObjectName(QStringLiteral("kcfg_ShowOSD"));
    layout->addRow(QString(), showOSD);

    // Checkbox: Notify about embedded files, forms, or signatures
    QCheckBox *showEmbeddedContentMessages = new QCheckBox(this);
    showEmbeddedContentMessages->setText(i18nc("@option:check Config dialog, general page", "Notify about embedded files, forms, or signatures"));
    showEmbeddedContentMessages->setObjectName(QStringLiteral("kcfg_ShowEmbeddedContentMessages"));
    layout->addRow(QString(), showEmbeddedContentMessages);

    if (embedMode != Okular::ViewerWidgetMode) {
        // Checkbox: display document title in titlebar
        QCheckBox *showTitle = new QCheckBox(this);
        showTitle->setText(i18nc("@option:check Config dialog, general page", "Display document title in titlebar if available"));
        showTitle->setObjectName(QStringLiteral("kcfg_DisplayDocumentTitle"));
        layout->addRow(QString(), showTitle);

        // Combobox: prefer file name or full path in titlebar
        QComboBox *nameOrPath = new QComboBox(this);
        nameOrPath->addItem(i18nc("item:inlistbox Config dialog, general page", "Display file name only"));
        nameOrPath->addItem(i18nc("item:inlistbox Config dialog, general page", "Display full file path"));
        nameOrPath->setObjectName(QStringLiteral("kcfg_DisplayDocumentNameOrPath"));
        layout->addRow(i18nc("label:listbox Config dialog, general page", "When not displaying document title:"), nameOrPath);
    }

    // Checkbox and color button: custom background color
    QCheckBox *useCustomColor = new QCheckBox(this);
    useCustomColor->setText(QString());
    useCustomColor->setObjectName(QStringLiteral("kcfg_UseCustomBackgroundColor"));

    KColorButton *customColor = new KColorButton(this);
    customColor->setObjectName(QStringLiteral("kcfg_BackgroundColor"));

    QHBoxLayout *customColorLayout = new QHBoxLayout();
    customColorLayout->addWidget(useCustomColor);
    useCustomColor->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    customColorLayout->addWidget(customColor);
    layout->addRow(i18nc("@label:listbox Config dialog, general page", "Use custom background color:"), customColorLayout);

    useCustomColor->setChecked(false);
    customColor->setEnabled(false);
    connect(useCustomColor, &QCheckBox::toggled, customColor, &QWidget::setEnabled);
    // END Appearance section

    layout->addRow(new QLabel(this));

    // BEGIN Program features section
    bool programFeaturesLabelPlaced = false;
    /**
     * This ensures the and only the first checkbox gets the “Program features:” label.
     *
     * (Right now I could move @c showBackendSelectionDialog to the first row,
     * but maybe someone wants to make it conditional too later.)
     */
    auto programFeaturesLabel = [&programFeaturesLabelPlaced]() -> QString {
        if (programFeaturesLabelPlaced) {
            return QString();
        } else {
            programFeaturesLabelPlaced = true;
            return i18nc("@title:group Config dialog, general page", "Program features:");
        }
    };

    if (embedMode == Okular::NativeShellMode) {
        // Two checkboxes: use tabs, switch to open tab
        QCheckBox *useTabs = new QCheckBox(this);
        useTabs->setText(i18nc("@option:check Config dialog, general page", "Open new files in tabs"));
        useTabs->setObjectName(QStringLiteral("kcfg_ShellOpenFileInTabs"));
        layout->addRow(programFeaturesLabel(), useTabs);

        QCheckBox *switchToTab = new QCheckBox(this);
        switchToTab->setText(i18nc("@option:check Config dialog, general page", "Switch to existing tab if file is already open"));
        switchToTab->setObjectName(QStringLiteral("kcfg_SwitchToTabIfOpen"));
        layout->addRow(QString(), switchToTab);

        useTabs->setChecked(false);
        switchToTab->setEnabled(false);
        connect(useTabs, &QCheckBox::toggled, switchToTab, &QWidget::setEnabled);
    }

#if !OKULAR_FORCE_DRM
    if (KAuthorized::authorize(QStringLiteral("skip_drm"))) {
        // Checkbox: Obey DRM
        QCheckBox *obeyDrm = new QCheckBox(this);
        obeyDrm->setText(i18nc("@option:check Config dialog, general page", "Obey DRM limitations"));
        obeyDrm->setObjectName(QStringLiteral("kcfg_ObeyDRM"));
        layout->addRow(programFeaturesLabel(), obeyDrm);
    }
#endif

    if (embedMode != Okular::ViewerWidgetMode) {
        // Checkbox: watch file for changes
        QCheckBox *watchFile = new QCheckBox(this);
        watchFile->setText(i18nc("@option:check Config dialog, general page", "Reload document on file change"));
        watchFile->setObjectName(QStringLiteral("kcfg_WatchFile"));
        layout->addRow(programFeaturesLabel(), watchFile);
    }

    // Checkbox: show backend selection dialog
    QCheckBox *showBackendSelectionDialog = new QCheckBox(this);
    showBackendSelectionDialog->setText(i18nc("@option:check Config dialog, general page", "Show backend selection dialog"));
    showBackendSelectionDialog->setObjectName(QStringLiteral("kcfg_ChooseGenerators"));
    layout->addRow(programFeaturesLabel(), showBackendSelectionDialog);

    if (embedMode != Okular::ViewerWidgetMode) { // TODO Makes sense?
        // Checkbox: RTL document layout
        QCheckBox *useRtl = new QCheckBox(this);
        useRtl->setText(i18nc("@option:check Config dialog, general page", "Right to left reading direction"));
        useRtl->setObjectName(QStringLiteral("kcfg_rtlReadingDirection"));
        layout->addRow(programFeaturesLabel(), useRtl);
    }

    QCheckBox *openInContinuousModeByDefault = new QCheckBox(this);
    openInContinuousModeByDefault->setText(i18nc("@option:check Config dialog, general page", "Open in continuous mode by default"));
    openInContinuousModeByDefault->setObjectName(QStringLiteral("kcfg_ViewContinuous"));
    layout->addRow(programFeaturesLabel(), openInContinuousModeByDefault);
    // END Program features section

    // If no Program features section, don’t add a second spacer:
    if (programFeaturesLabelPlaced) {
        layout->addRow(new QLabel(this));
    }

    // BEGIN View options section
    // Spinbox: overview columns
    QSpinBox *overviewColumns = new QSpinBox(this);
    overviewColumns->setMinimum(3);
    overviewColumns->setMaximum(10);
    overviewColumns->setObjectName(QStringLiteral("kcfg_ViewColumns"));
    layout->addRow(i18nc("@label:spinbox Config dialog, general page", "Overview columns:"), overviewColumns);

    // Spinbox: page up/down overlap
    QSpinBox *pageUpDownOverlap = new QSpinBox(this);
    pageUpDownOverlap->setMinimum(0);
    pageUpDownOverlap->setMaximum(50);
    pageUpDownOverlap->setSingleStep(5);
    pageUpDownOverlap->setSuffix(i18nc("Page Up/Down overlap, spinbox suffix", "%"));
    pageUpDownOverlap->setToolTip(i18nc("@info:tooltip Config dialog, general page", "Defines how much of the current viewing area will still be visible when pressing the Page Up/Down keys."));
    pageUpDownOverlap->setObjectName(QStringLiteral("kcfg_ScrollOverlap"));
    layout->addRow(i18nc("@label:spinbox Config dialog, general page", "Page Up/Down overlap:"), pageUpDownOverlap);

    // Combobox: prefer file name or full path in titlebar
    QComboBox *defaultZoom = new QComboBox(this);
    defaultZoom->addItem(i18nc("item:inlistbox Config dialog, general page, default zoom", "100%"));
    defaultZoom->addItem(i18nc("item:inlistbox Config dialog, general page, default zoom", "Fit Width"));
    defaultZoom->addItem(i18nc("item:inlistbox Config dialog, general page, default zoom", "Fit Page"));
    defaultZoom->addItem(i18nc("item:inlistbox Config dialog, general page, default zoom", "Auto Fit"));
    defaultZoom->setToolTip(i18nc("item:inlistbox Config dialog, general page, default zoom", "Defines the default zoom mode for files which were never opened before. For files which were opened before the previous zoom is applied."));
    defaultZoom->setObjectName(QStringLiteral("kcfg_ZoomMode"));
    layout->addRow(i18nc("label:listbox Config dialog, general page, default zoom", "Default zoom:"), defaultZoom);
    // END View options section
}
