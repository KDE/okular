/* This file was part of the KDE libraries (copied partially from kmenu.cpp)

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "okmenutitle.h"

#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QToolButton>

OKMenuTitle::OKMenuTitle(QMenu *menu, const QString &text, const QIcon &icon)
    : QWidgetAction(menu)
{
    QAction *buttonAction = new QAction(menu);
    QFont font = buttonAction->font();
    font.setBold(true);
    buttonAction->setFont(font);
    buttonAction->setText(text);
    buttonAction->setIcon(icon);

    QToolButton *titleButton = new QToolButton(menu);
    titleButton->installEventFilter(this); // prevent clicks on the title of the menu
    titleButton->setDefaultAction(buttonAction);
    titleButton->setDown(true); // prevent hover style changes in some styles
    titleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setDefaultWidget(titleButton);
}

bool OKMenuTitle::eventFilter(QObject *object, QEvent *event)
{
    Q_UNUSED(object);

    if (event->type() == QEvent::Paint) {
        return QWidgetAction::eventFilter(object, event);
    } else if (event->type() == QEvent::KeyRelease) {
        // If we're receiving the key release event is because we just gained
        // focus though a key event, use the same key to move it to the next action
        if (static_cast<QMenu *>(parentWidget())->activeAction() == this) {
            QKeyEvent *ke = static_cast<QKeyEvent *>(event);
            QKeyEvent newKe(QEvent::KeyPress, ke->key(), ke->modifiers(), ke->text(), ke->isAutoRepeat(), ke->count());
            QApplication::sendEvent(parentWidget(), &newKe);
        }

        // TODO What happens when there's multiple OKMenuTitle or only OKMenuTitle in a menu
    }

    event->accept();
    return true;
}
