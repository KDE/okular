/*
    SPDX-FileCopyrightText: 2019-2021 David Hurka <david.hurka@mailbox.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "colormodemenu.h"

#include <KActionCollection>
#include <KLocalizedString>
#include <kwidgetsaddons_version.h> // TODO KF6: Remove, this was needed for KActionMenu::setPopupMode().

#include "gui/guiutils.h"
#include "settings.h"

ColorModeMenu::ColorModeMenu(KActionCollection *ac, QObject *parent)
    : ToggleActionMenu(QIcon::fromTheme(QStringLiteral("color-management")), i18nc("@title:menu", "&Color Mode"), parent)
    , m_colorModeActionGroup(new QActionGroup(this))
    , m_aChangeColors(new KToggleAction(QIcon::fromTheme(QStringLiteral("color-management")), i18nc("@action Change Colors feature toggle action", "Change Colors"), this))
{
    setPopupMode(QToolButton::MenuButtonPopup);

    Q_ASSERT_X(ac->action(QStringLiteral("color_mode_menu")) == nullptr, "ColorModeMenu", "ColorModeMenu constructed twice; color_mode_menu already in action collection.");
    ac->addAction(QStringLiteral("color_mode_menu"), this);

    // Normal Colors action.
    m_aNormal = new KToggleAction(i18nc("@item:inmenu color mode", "&Normal Colors"), this);
    ac->addAction(QStringLiteral("color_mode_normal"), m_aNormal);
    addAction(m_aNormal);
    m_colorModeActionGroup->addAction(m_aNormal);

    // Other color mode actions.
    auto addColorMode = [=](KToggleAction *a, const QString &name, Okular::SettingsCore::EnumRenderMode::type id) {
        a->setData(int(id));
        addAction(a);
        ac->addAction(name, a);
        m_colorModeActionGroup->addAction(a);
    };
    addColorMode(new KToggleAction(QIcon::fromTheme(QStringLiteral("invertimage")), i18nc("@item:inmenu color mode", "&Invert Colors"), this), QStringLiteral("color_mode_inverted"), Okular::SettingsCore::EnumRenderMode::Inverted);
    m_aPaperColor = new KToggleAction(i18nc("@item:inmenu color mode", "Change &Paper Color"), this);
    addColorMode(m_aPaperColor, QStringLiteral("color_mode_paper"), Okular::SettingsCore::EnumRenderMode::Paper);
    m_aDarkLight = new KToggleAction(i18nc("@item:inmenu color mode", "Change &Dark && Light Colors"), this);
    addColorMode(m_aDarkLight, QStringLiteral("color_mode_recolor"), Okular::SettingsCore::EnumRenderMode::Recolor);
    addColorMode(new KToggleAction(QIcon::fromTheme(QStringLiteral("color-mode-black-white")), i18nc("@item:inmenu color mode", "Convert to &Black && White"), this),
                 QStringLiteral("color_mode_black_white"),
                 Okular::SettingsCore::EnumRenderMode::BlackWhite);
    addColorMode(new KToggleAction(QIcon::fromTheme(QStringLiteral("color-mode-invert-text")), i18nc("@item:inmenu color mode", "Invert &Lightness"), this),
                 QStringLiteral("color_mode_invert_lightness"),
                 Okular::SettingsCore::EnumRenderMode::InvertLightness);
    addColorMode(new KToggleAction(QIcon::fromTheme(QStringLiteral("color-mode-invert-image")), i18nc("@item:inmenu color mode", "Invert L&uma (sRGB Linear)"), this),
                 QStringLiteral("color_mode_invert_luma_srgb"),
                 Okular::SettingsCore::EnumRenderMode::InvertLuma);
    addColorMode(new KToggleAction(QIcon::fromTheme(QStringLiteral("color-mode-invert-image")), i18nc("@item:inmenu color mode", "Invert Luma (&Symmetric)"), this),
                 QStringLiteral("color_mode_invert_luma_symmetric"),
                 Okular::SettingsCore::EnumRenderMode::InvertLumaSymmetric);
    addColorMode(new KToggleAction(QIcon::fromTheme(QStringLiteral("color-mode-hue-shift-positive")), i18nc("@item:inmenu color mode", "Shift Hue P&ositive"), this),
                 QStringLiteral("color_mode_hue_shift_positive"),
                 Okular::SettingsCore::EnumRenderMode::HueShiftPositive);
    addColorMode(new KToggleAction(QIcon::fromTheme(QStringLiteral("color-mode-hue-shift-negative")), i18nc("@item:inmenu color mode", "Shift Hue N&egative"), this),
                 QStringLiteral("color_mode_hue_shift_negative"),
                 Okular::SettingsCore::EnumRenderMode::HueShiftNegative);

    // Add Configure Color Modes action.
    addSeparator();
    QAction *aConfigure = ac->action(QStringLiteral("options_configure_color_modes"));
    Q_ASSERT_X(aConfigure, "ColorModeMenu", "options_configure_color_modes is not in the action collection.");
    addAction(aConfigure);

    connect(m_colorModeActionGroup, &QActionGroup::triggered, this, &ColorModeMenu::slotColorModeActionTriggered);
    connect(Okular::SettingsCore::self(), &Okular::SettingsCore::colorModesChanged, this, &ColorModeMenu::slotConfigChanged);
    connect(Okular::Settings::self(), &Okular::Settings::colorModesChanged2, this, &ColorModeMenu::slotConfigChanged);
    connect(this, &QAction::changed, this, &ColorModeMenu::slotChanged);

    // Allow to configure a toggle shortcut.
    connect(m_aChangeColors, &QAction::toggled, this, &ColorModeMenu::slotSetChangeColors);
    ac->addAction(QStringLiteral("color_mode_change_colors"), m_aChangeColors);

    slotConfigChanged();
}

void ColorModeMenu::slotColorModeActionTriggered(QAction *action)
{
    const int newRenderMode = action->data().toInt();
    // Color mode toggles to normal when the currently checked mode is triggered.
    // Normal mode is special, triggering it always enables normal mode.
    // Otherwise, the triggered color mode is activated.
    if (action == m_aNormal) {
        Okular::SettingsCore::setChangeColors(false);
    } else if (Okular::SettingsCore::renderMode() == newRenderMode) {
        Okular::SettingsCore::setChangeColors(!Okular::SettingsCore::changeColors());
    } else {
        Okular::SettingsCore::setRenderMode(newRenderMode);
        Okular::SettingsCore::setChangeColors(true);
    }
    Okular::SettingsCore::self()->save();
}

void ColorModeMenu::slotSetChangeColors(bool on)
{
    Okular::SettingsCore::setChangeColors(on);
    Okular::SettingsCore::self()->save();
}

void ColorModeMenu::slotConfigChanged()
{
    // Check the current color mode action, and update the toolbar button default action
    const int rm = Okular::SettingsCore::renderMode();
    const QList<QAction *> colorModeActions = m_colorModeActionGroup->actions();
    for (QAction *a : colorModeActions) {
        if (a != m_aNormal && a->data().toInt() == rm) {
            a->setChecked(true);
            setDefaultAction(a);
            break;
        }
    }

    // If Change Colors is disabled, check Normal Colors instead
    if (!Okular::SettingsCore::changeColors()) {
        m_aNormal->setChecked(true);
    }

    // Update color icons
    m_aPaperColor->setIcon(GuiUtils::createColorIcon(QList<QColor>() << Okular::Settings::paperColor(), QIcon::fromTheme(QStringLiteral("paper-color"))));
    m_aDarkLight->setIcon(GuiUtils::createColorIcon(QList<QColor>() << Okular::Settings::recolorForeground() << Okular::Settings::recolorBackground(), QIcon::fromTheme(QStringLiteral("color-mode-black-white"))));

    // Update toggle action
    m_aChangeColors->setChecked(Okular::SettingsCore::changeColors());
}

void ColorModeMenu::slotChanged()
{
    const bool enabled = isEnabled();
    const QList<QAction *> colorModeActions = m_colorModeActionGroup->actions();
    for (QAction *a : colorModeActions) {
        a->setEnabled(enabled);
    }
}
