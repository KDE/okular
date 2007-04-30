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
#include <qhash.h>
#include <qtoolbutton.h>

#include <KIcon>

class QAction;
class QLabel;
class QTimer;
class FormWidgetIface;

namespace Okular {
class Page;
}

/**
 * @short PageViewItem represents graphically a page into the PageView.
 *
 * It has methods for settings Item's geometry and other visual properties such
 * as the individual zoom factor.
 */
class PageViewItem
{
    public:
        PageViewItem( const Okular::Page * page );
        ~PageViewItem();

        const Okular::Page * page() const;
        int pageNumber() const;
        const QRect& geometry() const;
        int width() const;
        int height() const;
        double zoomFactor() const;
        QHash<QString, FormWidgetIface*>& formWidgets();

        void setGeometry( int x, int y, int width, int height );
        void setWHZ( int w, int h, double zoom );
        void moveTo( int x, int y );
        void invalidate();
        bool setFormWidgetsVisible( bool visible );

    private:
        const Okular::Page * m_page;
        double m_zoomFactor;
        QRect m_geometry;
        QHash<QString, FormWidgetIface*> m_formWidgets;
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
 * @short A widget that displays messages in the top part of the page view.
 *
 * ...
 */
class PageViewTopMessage : public QWidget
{
    Q_OBJECT
    public:
        PageViewTopMessage( QWidget * parent );

        void setup( const QString & message, const KIcon& icon = KIcon() );
        void setIconSize( int size );
        void setActionButton( QAction * action );

    signals:
        void action();

    private:
        QLabel * m_label;
        QLabel * m_icon;
        QToolButton * m_button;
};


struct AnnotationItem
{
    AnnotationItem()
        : id( -1 ), isText( false )
    {
    }

    int id;
    QString text;
    QString pixmap;
    QString shortcut;
    bool isText;
};

class ToolBarButton : public QToolButton
{
    Q_OBJECT
    public:
        static const int iconSize = 32;
        static const int buttonSize = 40;

        ToolBarButton( QWidget * parent, const AnnotationItem &item );
        int buttonID() const { return m_id; }

    private:
        int m_id;
};

/**
 * @short A widget containing exclusive buttons, that slides in from a side.
 *
 * This is a shaped widget that slides in from a side of the 'anchor widget'
 * it's attached to. It can be dragged and docked on {left,top,right,bottom}
 * sides and contains toggable exclusive buttons.
 * When a 'tool' of this 'toolBar' is selected, a signal is emitted.
 */
class PageViewToolBar : public QWidget
{
    Q_OBJECT
    public:
        PageViewToolBar( QWidget * parent, QWidget * anchorWidget );
        ~PageViewToolBar();

        // animated widget controls
        enum Side { Left = 0, Top = 1, Right = 2, Bottom = 3 };

        void setItems( const QLinkedList<AnnotationItem> &items );
        void setSide( Side side );

        void showAndAnimate();
        void hideAndDestroy();

        // query properties

    signals:
        // the tool 'toolID' has been selected
        void toolSelected( int toolID );
        // orientation has been changed
        void orientationChanged( int side );

    protected:
        // handle widget events { anchor_resize, paint, animation, drag }
        bool eventFilter( QObject * o, QEvent * e );
        void paintEvent( QPaintEvent * );
        void mousePressEvent( QMouseEvent * e );
        void mouseMoveEvent( QMouseEvent * e );
        void mouseReleaseEvent( QMouseEvent * e );

    private:
        // private variables
        class ToolBarPrivate * d;

    private slots:
        void slotAnimate();
        void slotButtonClicked();
};

#endif
