/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _PAGEVIEW_UTILS_H
#define _PAGEVIEW_UTILS_H

#include <qwidget.h>
#include <qpixmap.h>
#include <qrect.h>

class QTimer;


/**
 * @short A widget that displays messages in the top-left corner.
 */
class PageViewMessage : public QWidget
{
    public:
        PageViewMessage( QWidget * parent );

        enum Icon { None, Info, Warning, Error };
        void display( const QString & message, Icon icon, int durationMs = -1 );

    protected:
        void paintEvent( QPaintEvent * e );
        void mouseMoveEvent( QMouseEvent * e );

    private:
        QPixmap m_pixmap;
        QTimer * m_timer;
};


/**
 * @short The overlay widget draws transparent selections over the page.
 */
class PageViewOverlay : public QWidget
{
    public:
        enum OverlayOperation { Zoom, SnapShot };
        PageViewOverlay( QWidget * parent, OverlayOperation op );

        void setBeginCorner( int x, int y );
        void setEndCorner( int x, int y );
        const QRect & selectedRect();

    protected:
        void paintEvent( QPaintEvent * e );

    private:
        QPixmap m_backPixmap;
        QRect m_oldRect;
        QRect m_currentRect;
        int m_startX;
        int m_startY;
};

#endif
