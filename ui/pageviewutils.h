/***************************************************************************
 *   Copyright (C) 2004-2005 by Enrico Ros <eros.kde@email.it>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _PAGEVIEW_UTILS_H_
#define _PAGEVIEW_UTILS_H_

#include <qwidget.h>
#include <qpixmap.h>
#include <qrect.h>

class QTimer;
class KPDFPage;

/**
 * @short PageViewItem represents graphically a kpdfpage into the PageView.
 *
 * It has methods for settings Item's geometry and other visual properties such
 * as the individual zoom factor.
 */
class PageViewItem
{
    public:
        PageViewItem( const KPDFPage * page );

        const KPDFPage * page() const;
        int pageNumber() const;
        const QRect& geometry() const;
        int width() const;
        int height() const;
        double zoomFactor() const;

        void setGeometry( int x, int y, int width, int height );
        void setWHZ( int w, int h, double zoom );
        void moveTo( int x, int y );

    private:
        const KPDFPage * m_page;
        double m_zoomFactor;
        QRect m_geometry;
};


/**
 * @short A widget that displays messages in the top-left corner.
 *
 * This is a widget with thin border and rounded corners that displays a given
 * text along as an icon. It's meant to be used for displaying messages to the
 * user by placing this above other widgets.
 */
class PageViewMessage : public QWidget
{
    public:
        PageViewMessage( QWidget * parent );

        enum Icon { None, Info, Warning, Error, Find, Annotation };
        void display( const QString & message, Icon icon = Info, int durationMs = 4000 );

    protected:
        void paintEvent( QPaintEvent * e );
        void mousePressEvent( QMouseEvent * e );

    private:
        QPixmap m_pixmap;
        QTimer * m_timer;
};


/**
 * @short A widget containing exclusive buttons, that slides in from a side.
 *
 * This is a shaped widget that slides in from a side of the 'anchor widget'
 * it's attached to. It can be dragged and docked on {left,top,right,bottom}
 * sides and contains toggable exclusive buttons.
 * When a 'tool' of this 'toolBar' is selected, a signal is emitted.
 */
struct ToolBarItem // FIXME TEMP: MOVE OUT OF HERE!
{
    int id;
    QString text;
    QString pixmap;

    ToolBarItem() {};
    ToolBarItem( int _id, const QString & _text, const QString & _pixmap )
        : id( _id ), text( _text ), pixmap( _pixmap ) {};
};

class PageViewToolBar : public QWidget
{
    Q_OBJECT
    public:
        PageViewToolBar( QWidget * parent, QWidget * anchorWidget );
        ~PageViewToolBar();

        // animated widget controls
        enum Side { Left = 0, Top = 1, Right = 2, Bottom = 3 };
        void showItems( Side side, const QValueList<ToolBarItem> & items );
        void hideAndDestroy();

    signals:
        // the tool 'toolID' has been selected
        void toolSelected( int toolID );

    protected:
        // handle widget events { anchor_resize, paint, animation, drag }
        bool eventFilter( QObject * o, QEvent * e );
        void paintEvent( QPaintEvent * );
        void mousePressEvent( QMouseEvent * e );
        void mouseMoveEvent( QMouseEvent * e );
        void mouseReleaseEvent( QMouseEvent * e );
        // used by subclasses to save configuration
        virtual void orientationChanged( Side /*side*/ ) {};

    private:
        // rebuild contents and reposition then widget
        void buildToolBar();
        void reposition();
        // compute the visible and hidden positions along current side
        QPoint getInnerPoint();
        QPoint getOuterPoint();

        // private variables
        class ToolBarPrivate * d;

    private slots:
        void slotAnimate();
        void slotButtonClicked();
};

class PageViewEditTools : public PageViewToolBar
{
    public:
        // constructs a toolbox filled up with edit tools
        PageViewEditTools( QWidget * parent, QWidget * anchorWidget );
        // hook call to save widget placement
        void orientationChanged( Side side );
};

#endif
