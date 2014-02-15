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

#include "core/area.h"

class QAction;
class QLabel;
class QTimer;
class FormWidgetIface;
class PageView;
class VideoWidget;

namespace Okular {
class Movie;
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
        double zoomFactor() const;
        bool isVisible() const;
        QHash<int, FormWidgetIface*>& formWidgets();
        QHash< Okular::Movie *, VideoWidget * >& videoWidgets();

        /* The page is cropped as follows: */
        const Okular::NormalizedRect & crop() const;

        /* Real geometry into which the cropped page is rendered: */
        const QRect& croppedGeometry() const;
        int croppedWidth() const;
        int croppedHeight() const;

        /* "Uncropped" geometry:
         * If the whole page was rendered into the uncropped geometry then the
         * cropped page would be rendered into the real geometry.
         * (Hence, uncropped always contains cropped, and they are equal only if
         * the page is uncropped.) This is just for convenience in calculations.
         */
        const QRect& uncroppedGeometry() const;
        int uncroppedWidth() const;
        int uncroppedHeight() const;

        /* Convert absolute geometry coordinates to normalized [0,1] page coordinates: */
        double absToPageX(double absX) const;
        double absToPageY(double absY) const;

        void setWHZC( int w, int h, double zoom, const Okular::NormalizedRect & c );
        void moveTo( int x, int y );
        void setVisible( bool visible );
        void invalidate();
        bool setFormWidgetsVisible( bool visible );

    private:
        const Okular::Page * m_page;
        double m_zoomFactor;
        bool m_visible;
        bool m_formsVisible;
        QRect m_croppedGeometry;
        QRect m_uncroppedGeometry;
        Okular::NormalizedRect m_crop;
        QHash<int, FormWidgetIface*> m_formWidgets;
        QHash< Okular::Movie *, VideoWidget * > m_videoWidgets;
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
        void display( const QString & message, const QString & details = QString(), Icon icon = Info, int durationMs = 4000 );

    protected:
        bool eventFilter(QObject * obj, QEvent * event );
        void paintEvent( QPaintEvent * e );
        void mousePressEvent( QMouseEvent * e );

    private:
        QRect computeTextRect( const QString & message, int extra_width ) const;
        void computeSizeAndResize();
        QString m_message;
        QString m_details;
        QPixmap m_symbol;
        QTimer * m_timer;
        int m_lineSpacing;
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


struct AnnotationToolItem
{
    AnnotationToolItem()
        : id( -1 ), isText( false )
    {
    }

    int id;
    QString text;
    QPixmap pixmap;
    QString shortcut;
    bool isText;
};

class ToolBarButton : public QToolButton
{
    Q_OBJECT
    public:
        static const int iconSize = 32;
        static const int buttonSize = 40;

        ToolBarButton( QWidget * parent, const AnnotationToolItem &item );
        int buttonID() const { return m_id; }
        bool isText() const { return m_isText; }

    signals:
        void buttonDoubleClicked( int buttonID );

    protected:
        void mouseDoubleClickEvent( QMouseEvent * event );

    private:
        int m_id;
        bool m_isText;
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
        PageViewToolBar( PageView * parent, QWidget * anchorWidget );
        ~PageViewToolBar();

        // animated widget controls
        enum Side { Left = 0, Top = 1, Right = 2, Bottom = 3 };

        void setItems( const QLinkedList<AnnotationToolItem> &items );
        void setSide( Side side );

        void showAndAnimate();
        void hideAndDestroy();

        void selectButton( int id );

        void setToolsEnabled( bool on );
        void setTextToolsEnabled( bool on );

        // query properties

    signals:
        // the tool 'toolID' has been selected
        void toolSelected( int toolID );
        // orientation has been changed
        void orientationChanged( int side );
        // a tool button of this toolbar has been double clicked
        void buttonDoubleClicked( int buttonID );

    protected:
        // handle widget events { anchor_resize, paint, animation, drag }
        bool eventFilter( QObject * o, QEvent * e );
        void paintEvent( QPaintEvent * );
        void mousePressEvent( QMouseEvent * e );
        void mouseMoveEvent( QMouseEvent * e );
        void mouseReleaseEvent( QMouseEvent * e );

    private:
        // private variables
        friend class ToolBarPrivate;
        class ToolBarPrivate * d;

    private slots:
        void slotAnimate();
        void slotButtonClicked();
};

#endif
