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

#include "toggleactionmenu.h"

#include <QMenu>
#include <QPointer>

ToggleActionMenu::ToggleActionMenu(QObject *parent)
    : ToggleActionMenu(QIcon(), QString(), parent)
{
}

ToggleActionMenu::ToggleActionMenu(const QString &text, QObject *parent)
    : ToggleActionMenu(QIcon(), text, parent)
{
}

ToggleActionMenu::ToggleActionMenu(const QIcon &icon, const QString &text, QObject *parent, PopupMode popupMode, MenuLogic logic)
    : KActionMenu(icon, text, parent)
    , m_defaultAction(nullptr)
    , m_suggestedDefaultAction(nullptr)
    , m_menuLogic(logic)
{
    connect(this, &QAction::changed, this, &ToggleActionMenu::updateButtons);

    if (popupMode == DelayedPopup) {
        setDelayed(true);
    } else {
        setDelayed(false);
    }
    setStickyMenu(false);

    if (logic & ImplicitDefaultAction) {
        connect(menu(), &QMenu::triggered, this, &ToggleActionMenu::setDefaultAction);
    }
}

QWidget *ToggleActionMenu::createWidget(QWidget *parent)
{
    QToolButton *button = qobject_cast<QToolButton *>(KActionMenu::createWidget(parent));
    if (!button) {
        // This function is used to add a button into the toolbar.
        // KActionMenu will plug itself as QToolButton.
        // So, if no QToolButton was returned, this was not called the intended way.
        return button;
    }

    // Remove this menu action from the button,
    // so it doesn't compose a menu of this menu action and its own menu.
    button->removeAction(this);
    // The button has lost the menu now, let it use the correct menu.
    button->setMenu(menu());

    m_buttons.append(QPointer<QToolButton>(button));

    // Apply other properties to the button.
    updateButtons();

    return button;
}

void ToggleActionMenu::setDefaultAction(QAction *action)
{
    m_defaultAction = action;
    updateButtons();
}

void ToggleActionMenu::suggestDefaultAction(QAction *action)
{
    m_suggestedDefaultAction = action;
}

QAction *ToggleActionMenu::checkedAction(QMenu *menu) const
{
    // Look at each action a in the menu whether it is checked.
    // If a is a menu, recursively call checkedAction().
    const QList<QAction *> actions = menu->actions();
    for (QAction *a : actions) {
        if (a->isChecked()) {
            return a;
        } else if (a->menu()) {
            QAction *b = checkedAction(a->menu());
            if (b) {
                return b;
            }
        }
    }
    return nullptr;
}

void ToggleActionMenu::updateButtons()
{
    for (const QPointer<QToolButton> &button : qAsConst(m_buttons)) {
        if (button) {
            button->setDefaultAction(defaultAction());

            // Override some properties of the default action,
            // where the property of this menu makes more sense.
            button->setEnabled(isEnabled());

            if (delayed()) {
                button->setPopupMode(QToolButton::DelayedPopup);
            } else if (stickyMenu()) {
                button->setPopupMode(QToolButton::InstantPopup);
            } else {
                button->setPopupMode(QToolButton::MenuButtonPopup);
            }
        }
    }
}

QAction *ToggleActionMenu::defaultAction()
{
    if ((m_menuLogic & ImplicitDefaultAction) && !m_defaultAction) {
        m_defaultAction = checkedAction(menu());
    }
    if (!m_defaultAction) {
        m_defaultAction = m_suggestedDefaultAction;
    }
    return m_defaultAction;
}
