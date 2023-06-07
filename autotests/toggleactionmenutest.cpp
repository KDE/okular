/*
    SPDX-FileCopyrightText: 2020 David Hurka <david.hurka@mailbox.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTest>

#include "../part/toggleactionmenu.h"
#include <QToolBar>

class ToggleActionMenuTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testSetDefaultAction();
    void testDeleteToolBarButton();
};

void ToggleActionMenuTest::testSetDefaultAction()
{
    QToolBar dummyToolBar;
    ToggleActionMenu menu(QStringLiteral("Menu"), this);
    QAction *actionA = new QAction(QStringLiteral("A"), this);
    QAction *actionB = new QAction(QStringLiteral("B"), this);

    // Do not set a default action, the menu should behave as plain KActionMenu.
    QCOMPARE(menu.defaultAction(), &menu);
    QToolButton *menuButton = qobject_cast<QToolButton *>(menu.createWidget(&dummyToolBar));
    QVERIFY(menuButton);
    QCOMPARE(menuButton->defaultAction(), &menu);

    // Should still behave as plain KActionMenu when actions are added.
    menu.addAction(actionA);
    QCOMPARE(menu.defaultAction(), &menu);
    QCOMPARE(menuButton->defaultAction(), &menu);

    // Set an action from the menu as default action, should work.
    menu.setDefaultAction(actionA);
    QCOMPARE(menu.defaultAction(), actionA);
    QCOMPARE(menuButton->defaultAction(), actionA);

    // Set a foreign action as default action, should reset the default action.
    menu.setDefaultAction(actionB);
    QCOMPARE(menu.defaultAction(), &menu);
    QCOMPARE(menuButton->defaultAction(), &menu);

    // Set an action of the menu as default action, should work.
    menu.setDefaultAction(actionA);
    QCOMPARE(menu.defaultAction(), actionA);
    QCOMPARE(menuButton->defaultAction(), actionA);

    // Remove default action from menu, should reset the default action.
    menu.removeAction(actionA);
    QCOMPARE(menu.defaultAction(), &menu);
    QCOMPARE(menuButton->defaultAction(), &menu);
}

void ToggleActionMenuTest::testDeleteToolBarButton()
{
    QToolBar dummyToolBar;
    ToggleActionMenu menu(QStringLiteral("Menu"), this);
    QAction *actionA = new QAction(QStringLiteral("A"), this);
    QAction *actionB = new QAction(QStringLiteral("B"), this);

    // Setup: set a default action and create two toolbar buttons.
    menu.addAction(actionA);
    menu.addAction(actionB);
    menu.setDefaultAction(actionA);
    QToolButton *menuButtonA = qobject_cast<QToolButton *>(menu.createWidget(&dummyToolBar));
    QVERIFY(menuButtonA);
    QCOMPARE(menuButtonA->defaultAction(), actionA);
    QToolButton *menuButtonB = qobject_cast<QToolButton *>(menu.createWidget(&dummyToolBar));
    QVERIFY(menuButtonB);

    // Delete button B, and set a new default action. Button A shall be updated without segfaulting on the deleted button B.
    delete menuButtonB;
    menu.setDefaultAction(actionB);
    QCOMPARE(menuButtonA->defaultAction(), actionB);
}

QTEST_MAIN(ToggleActionMenuTest)

#include "toggleactionmenutest.moc"
