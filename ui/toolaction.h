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

#include <kaction.h>

class QToolButton;

class ToolAction : public KAction
{
    Q_OBJECT

    public:
        ToolAction( QObject *parent = 0 );
        virtual ~ToolAction();

        void addAction( QAction *action );

    protected:
        virtual QWidget* createWidget( QWidget *parent );

    private slots:
        void slotNewDefaultAction( QAction *action );

    private:
        QList< QPointer< QToolButton > > m_buttons;
        QList< QAction * > m_actions;
};

#endif
