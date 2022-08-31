/*
    SPDX-FileCopyrightText: 2019-2021 David Hurka <david.hurka@mailbox.org>

    Inspired by and replacing toolaction.h by:
    SPDX-FileCopyrightText: 2004-2006 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef TOGGLEACTIONMENU_H
#define TOGGLEACTIONMENU_H

#include <KActionMenu>
#include <QPointer>
#include <QSet>
#include <QToolButton>

/**
 * @brief A KActionMenu, which allows to set the default action of its toolbar buttons.
 *
 * This behaves like a KActionMenu, with the addition of setDefaultAction().
 *
 * @par Intention
 * Setting the default action of toolbar buttons has the advantage that the user
 * can trigger a frequently used action directly without opening the menu.
 * Additionally, the state of the default action is visible in the toolbar.
 *
 * @par Example
 * You can make the toolbar button show the last used action with only one connection.
 * You may want to initialize the default action.
 * \code
 * if (myToggleActionMenu->defaultAction() == myToggleActionMenu) {
 *     myToggleActionMenu->setDefaultAction(myFirstAction);
 * }
 * connect(myToggleActionMenu->menu(), &QMenu::triggered,
 *         myToggleActionMenu, &ToggleActionMenu::setDefaultAction);
 * \endcode
 */
class ToggleActionMenu : public KActionMenu
{
    Q_OBJECT

public:
    explicit ToggleActionMenu(QObject *parent);
    ToggleActionMenu(const QString &text, QObject *parent);
    /**
     * Constructs an empty ToggleActionMenu.
     *
     * @param icon The icon of this menu, when plugged into another menu.
     * @param text The name of this menu, when plugged into another menu.
     * @param parent Parent @c QOject.
     */
    ToggleActionMenu(const QIcon &icon, const QString &text, QObject *parent);

    QWidget *createWidget(QWidget *parent) override;

    /**
     * Returns the current default action of the toolbar buttons.
     * May be @c this.
     *
     * This action is set by setDefaultAction().
     */
    QAction *defaultAction();

public Q_SLOTS:
    /**
     * Sets the default action of the toolbar buttons.
     *
     * Toolbar buttons are updated immediately.
     *
     * Calling setDefaultAction(nullptr) will reset the default action
     * to this menu itself.
     *
     * @note
     * @p action must be present in the menu as direct child action.
     * The default action will be reset to this menu itself
     * when @p action is removed from the menu.
     *
     * @note
     * @p action will define all properties of the toolbar buttons.
     * When you disable @p action, the toolbar button will become disabled too.
     * Then the menu can no longer be accessed.
     */
    void setDefaultAction(QAction *action);

protected:
    /** Can store @c nullptr, which means this menu itself will be the default action. */
    QPointer<QAction> m_defaultAction;
    QList<QPointer<QToolButton>> m_buttons;

    QHash<const QToolButton *, Qt::ToolButtonStyle> m_originalToolButtonStyle;

    /**
     * Returns the aproppriate style for @p button.
     * Respects both toolbar settings and settings for this menu action.
     */
    Qt::ToolButtonStyle styleFor(QToolButton *button) const;

    /**
     * Updates the toolbar buttons by setting the current defaultAction() on them.
     *
     * (If the current defaultAction() is invalid, `this` is used instead.)
     */
    void updateButtons();

    /**
     * Updates the event filter, which listens to QMenu’s QActionEvent.
     *
     * This is connected to QAction::changed().
     * That signal is emmited when the menu changes, but that’s not documented.
     */
    void slotMenuChanged();

    bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif // TOGGLEACTIONMENU_H
