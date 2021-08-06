/*
    SPDX-FileCopyrightText: 2019-2021 David Hurka <david.hurka@mailbox.org>

    Inspired by and replacing toolaction.h by:
    SPDX-FileCopyrightText: 2004-2006 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "toggleactionmenu.h"

#include <QActionEvent>
#include <QMenu>

ToggleActionMenu::ToggleActionMenu(QObject *parent)
    : ToggleActionMenu(QIcon(), QString(), parent)
{
}

ToggleActionMenu::ToggleActionMenu(const QString &text, QObject *parent)
    : ToggleActionMenu(QIcon(), text, parent)
{
}

ToggleActionMenu::ToggleActionMenu(const QIcon &icon, const QString &text, QObject *parent)
    : KActionMenu(icon, text, parent)
    , m_defaultAction(nullptr)
{
    slotMenuChanged();
}

QWidget *ToggleActionMenu::createWidget(QWidget *parent)
{
    QWidget *buttonWidget = KActionMenu::createWidget(parent);
    QToolButton *button = qobject_cast<QToolButton *>(buttonWidget);
    if (!button) {
        // This function is used to add a button into the toolbar.
        // QWidgetAction::createWidget() is also called with other parents,
        // e. g. when this ToggleActionMenu is added to a QMenu.
        // Therefore, reaching this code path is a valid use case.
        return buttonWidget;
    }

    // BEGIN QToolButton hack
    // Setting the default action of a QToolButton
    // to an action of its menu() is tricky.
    // Remove this menu action from the button,
    // so it doesn't compose a menu of this menu action and its own menu.
    button->removeAction(this);
    // The button has lost the menu now, let it use the correct menu.
    button->setMenu(menu());
    // END QToolButton hack

    m_buttons.append(button);

    // Apply other properties to the button.
    updateButtons();

    return button;
}

QAction *ToggleActionMenu::defaultAction()
{
    return m_defaultAction ? m_defaultAction : this;
}

void ToggleActionMenu::setDefaultAction(QAction *action)
{
    if (action && menu()->actions().contains(action)) {
        m_defaultAction = action;
    } else {
        m_defaultAction = nullptr;
    }
    updateButtons();
}

void ToggleActionMenu::updateButtons()
{
    for (QToolButton *button : qAsConst(m_buttons)) {
        if (button) {
            button->setDefaultAction(this->defaultAction());

            if (delayed()) { // TODO deprecated interface.
                button->setPopupMode(QToolButton::DelayedPopup);
            } else if (stickyMenu()) {
                button->setPopupMode(QToolButton::InstantPopup);
            } else {
                button->setPopupMode(QToolButton::MenuButtonPopup);
            }
        }
    }
}

bool ToggleActionMenu::eventFilter(QObject *watched, QEvent *event)
{
    // If the defaultAction() is removed from the menu, reset the default action.
    if (watched == menu() && event->type() == QEvent::ActionRemoved) {
        QActionEvent *actionEvent = static_cast<QActionEvent *>(event);
        if (actionEvent->action() == defaultAction()) {
            setDefaultAction(nullptr);
        }
    }

    return KActionMenu::eventFilter(watched, event);
}

void ToggleActionMenu::slotMenuChanged()
{
    menu()->installEventFilter(this);
    // Not removing old event filter, because we would need to remember the old menu.
}

#include "moc_toggleactionmenu.cpp"
