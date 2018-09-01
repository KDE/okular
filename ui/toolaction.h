/***************************************************************************
 *   Copyright (C) 2004-2006 by Albert Astals Cid <aacid@kde.org>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef TOOLACTION_H
#define TOOLACTION_H

#include <qlist.h>
#include <qpointer.h>

#include <KSelectAction>

class QToolButton;

class ToolAction : public KSelectAction
{
    Q_OBJECT

    public:
        explicit ToolAction( QObject *parent = nullptr );
        virtual ~ToolAction();

        void addAction( QAction *action );

    protected:
        QWidget* createWidget( QWidget *parent ) override;

    private Q_SLOTS:
        void slotNewDefaultAction( QAction *action );

    private:
        QList< QPointer< QToolButton > > m_buttons;
        QList< QAction * > m_actions;
};

#endif
