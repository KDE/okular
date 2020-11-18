/***************************************************************************
 *   Copyright (C) 2019 by David Hurka <david.hurka@mailbox.org>           *
 *                                                                         *
 *   Inspired by and replacing toolaction.h by:                            *
 *     Copyright (C) 2004-2006 by Albert Astals Cid <aacid@kde.org>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef TOGGLEACTIONMENU_H
#define TOGGLEACTIONMENU_H

#include <KActionMenu>
#include <QSet>
#include <QToolButton>

/**
 * @brief A KActionMenu, with allows to set the default action of its toolbar buttons.
 *
 * Usually, a KActionMenu creates toolbar buttons which reflect its own action properties
 * (icon, text, tooltip, checked state,...), as it is a QAction itself.
 *
 * ToggleActionMenu will use its own action properties only when plugged as submenu in another menu.
 * The default action of the toolbar buttons can easily be changed with the slot setDefaultAction().
 *
 * Naming: The user can *Toggle* the checked state of an *Action* by directly clicking the toolbar button,
 * but can also open a *Menu*.
 *
 * @par Intention
 * Setting the default action of the toolbar button can be useful for:
 *  * Providing the most probably needed entry of a menu directly on the menu button.
 *  * Showing the last used menu entry on the menu button, including its checked state.
 * The advantage is that the user often does not need to open the menu,
 * and that the toolbar button shows additional information
 * like checked state or the user's last selection.
 *
 * This shall replace the former ToolAction in Okular,
 * while being flexible enough for other (planned) action menus.
 */
class ToggleActionMenu : public KActionMenu
{
    Q_OBJECT

public:
    /**
     * Defines how the menu behaves.
     */
    enum MenuLogic {
        DefaultLogic = 0x0,
        /**
         * Automatically makes the triggered action the default action, even if in a submenu.
         * When a toolbar button is constructed,
         * the default action is set to the default action set with setDefaultAction() before,
         * otherwise to the first checked action in the menu,
         * otherwise to the action suggested with suggestDefaultAction().
         */
        ImplicitDefaultAction = 0x1
    };

    enum PopupMode { DelayedPopup, MenuButtonPopup };

    explicit ToggleActionMenu(QObject *parent);
    ToggleActionMenu(const QString &text, QObject *parent);
    /**
     * Constructs an empty ToggleActionMenu.
     *
     * @param icon The icon of this menu, when plugged into another menu.
     * @param text The name of this menu, when plugged into another menu.
     * @param parent Parent @c QOject.
     * @param popupMode The popup mode of the toolbar buttons.
     * You will want to use @c DelayedPopup or @c MenuButtonPopup,
     * @c InstantPopup would make @c ToggleActionMenu pointless.
     * @param logic To define special behaviour of @c ToggleActionMenu,
     * to simplify the usage.
     */
    ToggleActionMenu(const QIcon &icon, const QString &text, QObject *parent, PopupMode popupMode = MenuButtonPopup, MenuLogic logic = DefaultLogic);

    QWidget *createWidget(QWidget *parent) override;

    /**
     * Returns the current default action of the toolbar buttons.
     *
     * In ImplicitDefaultAction mode,
     * when the default action was not yet set with setDefaultAction(),
     * it will determine it from the first checked action in the menu,
     * otherwise from the action set with suggestDefaultAction().
     */
    QAction *defaultAction();

    /**
     * Suggests a default action to be used as fallback.
     *
     * It will be used if the default action is not determined another way.
     * This is useful for ImplicitDefaultAction mode,
     * when you can not guarantee that one action in the menu
     * will be checked.
     *
     * @note
     * In DefaultLogic mode, or when you already have called setDefaultAction(),
     * you have to use setDefaultAction() instead.
     */
    void suggestDefaultAction(QAction *action);

public slots:
    /**
     * Sets the default action of the toolbar buttons.
     *
     * This action will be triggered by clicking directly on the toolbar buttons.
     * It will also set the text, icon, checked state, etc. of the toolbar buttons.
     *
     * @note
     * The default action will not set the enabled state or popup mode of the menu buttons.
     * These properties are still set by the corresponding properties of this ToggleActionMenu.
     *
     * @warning
     * The action will not be added to the menu,
     * it usually makes sense to addAction() it before to setDefaultAction() it.
     *
     * @see suggestDefaultAction()
     */
    void setDefaultAction(QAction *action);

private:
    QAction *m_defaultAction;
    QAction *m_suggestedDefaultAction;
    QList<QPointer<QToolButton>> m_buttons;
    MenuLogic m_menuLogic;

    /**
     * Returns the first checked action in @p menu and its submenus,
     * or nullptr if no action is checked.
     */
    QAction *checkedAction(QMenu *menu) const;

private slots:
    /**
     * Updates the toolbar buttons, using both the default action and properties of this menu itself.
     *
     * This ensures that the toolbar buttons reflect e. g. a disabled state of this menu.
     */
    void updateButtons();
};

#endif // TOGGLEACTIONMENU_H
