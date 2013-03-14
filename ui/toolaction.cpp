/***************************************************************************
 *   Copyright (C) 2004-2006 by Albert Astals Cid <aacid@kde.org>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "toolaction.h"

#include <qmenu.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>

#include <klocale.h>

ToolAction::ToolAction( QObject *parent )
    : KAction( parent )
{
    setText( i18n( "Selection Tools" ) );
}

ToolAction::~ToolAction()
{
}

void ToolAction::addAction( QAction *action )
{
    bool setDefault = !m_buttons.isEmpty() ? m_buttons.first()->menu()->actions().isEmpty() : false;
    foreach ( QToolButton *button, m_buttons )
        if ( button )
        {
            button->menu()->addAction( action );
            if ( setDefault )
                button->setDefaultAction( action );
        }
    m_actions.append( action );
}

QWidget* ToolAction::createWidget( QWidget *parent )
{
    QToolBar *toolBar = qobject_cast< QToolBar * >( parent );
    if ( !toolBar )
        return 0;

    QToolButton *button = new QToolButton( toolBar );
    button->setAutoRaise( true );
    button->setFocusPolicy( Qt::NoFocus );
    button->setIconSize( toolBar->iconSize() );
    button->setToolButtonStyle( toolBar->toolButtonStyle() );
    button->setPopupMode( QToolButton::DelayedPopup );
    button->setMenu( new QMenu( button ) );
    button->setCheckable( true );
    connect( toolBar, SIGNAL(iconSizeChanged(QSize)),
             button, SLOT(setIconSize(QSize)) );
    connect( toolBar, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)),
             button, SLOT(setToolButtonStyle(Qt::ToolButtonStyle)) );
    connect( button, SIGNAL(triggered(QAction*)), toolBar, SIGNAL(actionTriggered(QAction*)) );
    connect( button->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotNewDefaultAction(QAction*)) );

    m_buttons.append( button );

    if ( !m_actions.isEmpty() )
    {
        button->setDefaultAction( m_actions.first() );
        foreach ( QAction *action, m_actions )
        {
            button->menu()->addAction( action );
            if ( action->isChecked() )
                button->setDefaultAction( action );
        }
        button->setToolTip( i18n("Click to use the current selection tool\nClick and hold to choose another selection tool") );
    }

    return button;
}

void ToolAction::slotNewDefaultAction( QAction *action )
{
    foreach ( QToolButton *button, m_buttons )
        if ( button )
        {
            button->setDefaultAction( action );
            button->setToolTip( i18n("Click to use the current selection tool\nClick and hold to choose another selection tool") );
        }
}

#include "toolaction.moc"
