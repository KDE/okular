/***************************************************************************
 *   Copyright (C) 2020 by Simone Gaiarin <simgunz@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef ACTIONBAR_H
#define ACTIONBAR_H

#include <QWidgetAction>

class QAction;
class QWidget;

/**
 * @short A widget action to display a set of actions in a toolbar
 */
class ActionBar : public QWidgetAction
{
    Q_OBJECT

public:
    explicit ActionBar(QObject *parent = nullptr);
    QWidget *createWidget(QWidget *parent) override;

    void addAction(QAction *action);
    void insertAction(int pos, QAction *action);
    void removeAction(QAction *action);
    void recreateWidgets();

private:
    QList<QAction *> m_actions;
};

#endif
