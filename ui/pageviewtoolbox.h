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
        PageViewToolbox( QWidget * parent, QWidget * anchorWidget, bool isVertical );

        void setItems( const QValueList<ToolboxItem> & items );
        void show();
        void hideAndDestroy();
        void anchorChanged();

        void paintEvent( QPaintEvent * );
        void timerEvent( QTimerEvent * );

    signals:
        void toolSelected( int toolID );

    private:
        QPoint getStartPoint();
        QPoint getEndPoint();
        void buildGfx();

        QWidget * m_anchor;
        bool m_vertical;
        QPoint m_currentPosition;
        QPoint m_endPosition;
        int m_timerID;
        bool m_hiding;
        QPixmap m_pixmap;
        QValueList< ToolboxButton * > m_buttons;

    private slots:
        void slotButtonClicked();
};

class PageViewEditTools : public PageViewToolbox
{
    public:
        PageViewEditTools( QWidget * parent, QWidget * anchorWidget );

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

#endif
