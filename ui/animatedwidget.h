/***************************************************************************
 *   Copyright (C) 2009 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef ANIMATEDWIDGET_H
#define ANIMATEDWIDGET_H

#include <qbasictimer.h>
#include <qpixmap.h>
#include <qwidget.h>

class AnimatedWidget : public QWidget
{
    Q_OBJECT

    public:
        AnimatedWidget( const QString& iconName, QWidget *parent = 0 );
        virtual ~AnimatedWidget();

    public slots:
        void start();
        void stop();

    protected:
        void paintEvent( QPaintEvent *event );
        void resizeEvent( QResizeEvent *event );
        void timerEvent( QTimerEvent *event );

    private:
        void load();

        QString m_icon;
        QPixmap m_pixmap;
        int m_frames;
        int m_currentFrame;
        int m_size;
        QBasicTimer m_timer;
};

#endif
