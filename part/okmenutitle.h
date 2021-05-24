/* This file was part of the KDE libraries (copied partially from kmenu.cpp)

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef OKMENUTITLE_H
#define OKMENUTITLE_H

#include <QWidgetAction>

class OKMenuTitle : public QWidgetAction
{
    Q_OBJECT
public:
    OKMenuTitle(QMenu *menu, const QString &text, const QIcon &icon = QIcon());

    bool eventFilter(QObject *object, QEvent *event) override;
};

#endif
