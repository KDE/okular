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

#include <KLocalizedString>

ToolAction::ToolAction( QObject *parent )
    : KSelectAction( parent )
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
        return nullptr;

    QToolButton *button = new QToolButton( toolBar );
    button->setAutoRaise( true );
    button->setFocusPolicy( Qt::NoFocus );
    button->setIconSize( toolBar->iconSize() );
    button->setToolButtonStyle( toolBar->toolButtonStyle() );
    button->setPopupMode( QToolButton::MenuButtonPopup );
    button->setMenu( new QMenu( button ) );
    button->setCheckable( true );
    connect(toolBar, &QToolBar::iconSizeChanged, button, &QToolButton::setIconSize);
    connect(toolBar, &QToolBar::toolButtonStyleChanged, button, &QToolButton::setToolButtonStyle);
    connect(button, &QToolButton::triggered, toolBar, &QToolBar::actionTriggered);
    connect( button->menu(), &QMenu::triggered, this, &ToolAction::slotNewDefaultAction );

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
        button->setToolTip( i18n("Click to use the current selection tool\nClick on the arrow to choose another selection tool") );
    }

    return button;
}

void ToolAction::slotNewDefaultAction( QAction *action )
{
    foreach ( QToolButton *button, m_buttons )
        if ( button )
        {
            button->setDefaultAction( action );
            button->setToolTip( i18n("Click to use the current selection tool\nClick on the arrow to choose another selection tool") );
        }
}

#include "moc_toolaction.cpp"
