/*
    SPDX-FileCopyrightText: 2019-2021 David Hurka <david.hurka@mailbox.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef COLORMODEMENU_H
#define COLORMODEMENU_H

#include "toggleactionmenu.h"

class KActionCollection;
class KToggleAction;

/**
 * Color Mode menu. Allows to change Okular::Settings::RenderMode from the toolbar.
 *
 * The toolbar button will always show the last selected color mode (except normal mode),
 * so it can be quickly enabled and disabled by just clicking the button.
 * Clicking on the menu arrow opens a menu with all color modes (including normal mode),
 * and an action to configure the color modes.
 *
 * Every color mode actions is available in the action collection, in addition to this menu itself.
 *
 * Color mode actions are enabled/disabled automatically when this menu is enabled/disabled.
 */
class ColorModeMenu : public ToggleActionMenu
{
    Q_OBJECT

public:
    explicit ColorModeMenu(KActionCollection *ac, QObject *parent);

protected:
    /** Makes color mode actions exclusive */
    QActionGroup *m_colorModeActionGroup;

    KToggleAction *m_aNormal;
    KToggleAction *m_aPaperColor;
    KToggleAction *m_aDarkLight;

    /** Allows to set a shortcut to toggle the Change Colors feature. */
    KToggleAction *m_aChangeColors;

protected Q_SLOTS:
    /**
     * Sets the color mode (render mode) to the one represented by @p action.
     *
     * If @p action represents the current mode, toggles the Change Colors feature.
     */
    void slotColorModeActionTriggered(QAction *action);

    /**
     * Sets the change colors feature on or off.
     */
    void slotSetChangeColors(bool on);

    /**
     * Updates the default action and the checked states of the color mode menu.
     *
     * Call this when the color mode was changed or Change Colors was toggled.
     */
    void slotConfigChanged();

    /**
     * Updates child actions as necessary
     */
    void slotChanged();
};

#endif // COLORMODEMENU_H
