/***************************************************************************
 *   Copyright (C) 2020 by Simone Gaiarin <simgunz@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include "actionbar.h"

class ActionBarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ActionBarWidget(QToolBar *parent);
    void recreateButtons(const QList<QAction *> &actions);

private Q_SLOTS:
    void onOrientationChanged(Qt::Orientation orientation);
};

ActionBarWidget::ActionBarWidget(QToolBar *parent)
    : QWidget::QWidget(parent)
{
    QLayout *layout;
    if (parent->orientation() == Qt::Vertical) {
        layout = new QVBoxLayout();
    } else {
        layout = new QHBoxLayout();
    }
    setLayout(layout);
    connect(parent, &QToolBar::orientationChanged, this, &ActionBarWidget::onOrientationChanged);
}

void ActionBarWidget::recreateButtons(const QList<QAction *> &actions)
{
    QToolBar *parentToolbar = qobject_cast<QToolBar *>(parentWidget());
    if (!parentToolbar) {
        return;
    }
    for (auto &toolButton : findChildren<QToolButton *>()) {
        layout()->removeWidget(toolButton);
        delete toolButton;
    }
    for (const auto &action : actions) {
        QToolButton *toolButton = new QToolButton(this);
        toolButton->setAutoRaise(true);
        toolButton->setFocusPolicy(Qt::NoFocus);
        toolButton->setIconSize(parentToolbar->iconSize());
        toolButton->setToolButtonStyle(parentToolbar->toolButtonStyle());
        toolButton->setDefaultAction(action);
        layout()->addWidget(toolButton);
        layout()->setAlignment(toolButton, Qt::AlignCenter);
        connect(parentToolbar, &QToolBar::iconSizeChanged, toolButton, &QToolButton::setIconSize);
        connect(parentToolbar, &QToolBar::toolButtonStyleChanged, toolButton, &QToolButton::setToolButtonStyle);
    }
}

void ActionBarWidget::onOrientationChanged(Qt::Orientation orientation)
{
    QLayout *newLayout;
    if (orientation == Qt::Vertical) {
        newLayout = new QVBoxLayout();
    } else {
        newLayout = new QHBoxLayout();
    }
    QLayout *oldLayout = layout();
    for (auto &toolButton : findChildren<QToolButton *>()) {
        oldLayout->removeWidget(toolButton);
        newLayout->addWidget(toolButton);
        newLayout->setAlignment(toolButton, Qt::AlignCenter);
    }
    delete oldLayout;
    setLayout(newLayout);
}

ActionBar::ActionBar(QObject *parent)
    : QWidgetAction(parent)
{
}

QWidget *ActionBar::createWidget(QWidget *parent)
{
    QToolBar *parentToolbar = qobject_cast<QToolBar *>(parent);
    if (!parentToolbar) {
        return new QWidget();
    }
    ActionBarWidget *widget = new ActionBarWidget(parentToolbar);
    widget->recreateButtons(m_actions);
    return widget;
}

void ActionBar::addAction(QAction *action)
{
    m_actions.append(action);
}

void ActionBar::insertAction(int pos, QAction *action)
{
    m_actions.insert(pos, action);
}

void ActionBar::removeAction(QAction *action)
{
    m_actions.removeAll(action);
}

void ActionBar::recreateWidgets()
{
    const auto widgets = createdWidgets();
    for (auto *widget : widgets) {
        auto *actionBarWidget = qobject_cast<ActionBarWidget *>(widget);
        if (actionBarWidget) {
            actionBarWidget->recreateButtons(m_actions);
        }
    }
}

#include "actionbar.moc"
