/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_PAGEVIEWTOOLBOX_H_
#define _KPDF_PAGEVIEWTOOLBOX_H_

#include <qpushbutton.h>

class ToolboxItem;
class ToolboxButton;

/**
 * @short A widget (containing exclusive buttons) that slides in from left/top.
 */
class PageViewToolbox : public QWidget
{
    Q_OBJECT
    public:
        enum Side { Left = 0, Top = 1, Right = 2, Bottom = 3 };
        PageViewToolbox( QWidget * parent, QWidget * anchorWidget );

        // animate widget displaying with given contents
        void showItems( Side side, const QValueList<ToolboxItem> & items );
        // animate widget hiding and destroy itself
        void hideAndDestroy();
        // call this function to reposition the widget
        void anchorChanged();

    signals:
        // the tool 'toolID' has been selected
        void toolSelected( int toolID );

    protected:
        // handle widget events { paint, animation, dragging }
        void mousePressEvent( QMouseEvent * e );
        void mouseMoveEvent( QMouseEvent * e );
        void mouseReleaseEvent( QMouseEvent * e );
        void paintEvent( QPaintEvent * );
        void timerEvent( QTimerEvent * );
        // used by subclasses to save configuration
        virtual void orientationChanged( Side /*side*/ ) {};

    private:
        // show widget full-sized in its place
        void showFinal();
        // build widget contents
        void buildGfx();
        // get the ending point of transition
        QPoint getEndPoint();
        // get the place from which transition starts
        QPoint getStartPoint();

        // private variables
        QWidget * m_anchor;
        Side m_side;
        QPoint m_currentPosition;
        QPoint m_endPosition;
        int m_timerID;
        bool m_hiding;
        QPixmap m_pixmap;
        QValueList< ToolboxButton * > m_buttons;

    private slots:
        // send 'toolSelected' signal when a button is pressed
        void slotButtonClicked();
};

struct ToolboxItem
{
    int id;
    QString text;
    QString pixmap;

    ToolboxItem() {};
    ToolboxItem( int _id, const QString & _text, const QString & _pixmap )
    : id( _id ), text( _text ), pixmap( _pixmap ) {};
};


class PageViewEditTools : public PageViewToolbox
{
    public:
        // constructs a toolbox filled up with edit tools
        PageViewEditTools( QWidget * parent, QWidget * anchorWidget );
        // hook call to save widget placement
        void orientationChanged( Side side );
};

#endif
