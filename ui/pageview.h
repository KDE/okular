/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   With portions of code from kpdf/kpdf_pagewidget.h by:                 *
 *     Copyright (C) 2002 by Wilco Greven <greven@kde.org>                 *
 *     Copyright (C) 2003 by Christophe Devriese                           *
 *                           <Christophe.Devriese@student.kuleuven.ac.be>  *
 *     Copyright (C) 2003 by Laurent Montel <montel@kde.org>               *
 *     Copyright (C) 2003 by Kurt Pfeifle <kpfeifle@danka.de>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
// This file follows coding style described in kdebase/kicker/HACKING

#ifndef _KPDF_PAGEVIEW_H_
#define _KPDF_PAGEVIEW_H_

#include <qscrollview.h>
#include <qvaluevector.h>
#include "core/observer.h"

class KURL;
class KActionCollection;

class KPDFDocument;
class PageViewItem;
class PageViewPrivate;
class PageViewTip;

/**
 * @short The main view. Handles zoom and continuous mode.. oh, and page
 * @short display of course :-)
 * ...
 */
class PageView : public QScrollView, public DocumentObserver
{
    Q_OBJECT

        friend class PageViewTip;

    public:
        PageView( QWidget *parent, KPDFDocument *document );
        ~PageView();

        // Zoom mode ( last 4 are internally used only! )
        enum ZoomMode { ZoomFixed, ZoomFitWidth, ZoomFitPage, ZoomFitText,
                        ZoomIn, ZoomOut, ZoomRefreshCurrent };
        enum MouseMode { MouseNormal, MouseZoom, MouseSelect, MouseEdit };

        // create actions that interact with this widget
        void setupActions( KActionCollection * collection );

        // used from RMB menu
        bool canFitPageWidth();
        void fitPageWidth( int page );

        // inherited from DocumentObserver
        uint observerId() const { return PAGEVIEW_ID; }
        void notifySetup( const QValueVector< KPDFPage * > & pages, bool documentChanged );
        void notifyViewportChanged( bool smoothMove );
        void notifyPageChanged( int pageNumber, int changedFlags );
        void notifyContentsCleared( int changedFlags );
        bool canUnloadPixmap( int pageNum );

        void showText( const QString &text, int ms );

    signals:
        void urlDropped( const KURL& );
        void rightClick( const KPDFPage *, const QPoint & );

    protected:
        // main draw loop, draws pageViews on viewport
        void viewportPaintEvent( QPaintEvent * pe );
        void viewportResizeEvent( QResizeEvent* );

        // mouse / keyboard events
        void keyPressEvent( QKeyEvent* );
        void imEndEvent( QIMEvent * );
        void contentsMouseMoveEvent( QMouseEvent* );
        void contentsMousePressEvent( QMouseEvent* );
        void contentsMouseReleaseEvent( QMouseEvent* );
        void wheelEvent( QWheelEvent* );

        // drag and drop related events
        void dragEnterEvent( QDragEnterEvent* );
        void dropEvent( QDropEvent* );

    private:
        // draw items on the opened qpainter
        void paintItems( QPainter * p, const QRect & clipRect );
        // update item width and height using current zoom parameters
        void updateItemSize( PageViewItem * item, int columnWidth, int rowHeight );
        // return the widget placed on a certain point or 0 if clicking on empty space
        PageViewItem * pickItemOnPoint( int x, int y );
        // start / modify / clear selection rectangle
        void selectionStart( int x, int y, const QColor & color, bool aboveAll = false );
        void selectionEndPoint( int x, int y );
        void selectionClear();
        // update internal zoom values and end in a slotRelayoutPages();
        void updateZoom( ZoomMode newZm );
        // update the text on the label using global zoom value or current page's one
        void updateZoomText();
        // updates cursor
        void updateCursor( const QPoint &p );
        // does the type ahead search
        void doTypeAheadSearch();

        // don't want to expose classes in here
        class PageViewPrivate * d;

    private slots:
        // activated either directly or via QTimer on the viewportResizeEvent
        void slotRelayoutPages();
        // activated either directly or via the contentsMoving(int,int) signal
        void slotRequestVisiblePixmaps( int left = -1, int top = -1 );
        // activated by the viewport move timer
        void slotMoveViewport();
        // activated by the autoscroll timer (Shift+Up/Down keys)
        void slotAutoScoll();
        // activated by the dragScroll timer
        void slotDragScroll();
        // type-ahead find timeout
        void findAheadStop();
        // show the welcome message
        void slotShowWelcome();

        // connected to local actions (toolbar, menu, ..)
        void slotZoom();
        void slotZoomIn();
        void slotZoomOut();
        void slotFitToWidthToggled( bool );
        void slotFitToPageToggled( bool );
        void slotFitToTextToggled( bool );
        void slotTwoPagesToggled( bool );
        void slotContinuousToggled( bool );
        void slotSetMouseNormal();
        void slotSetMouseZoom();
        void slotSetMouseSelect();
        void slotSetMouseDraw();
        void slotScrollUp();
        void slotScrollDown();
};

#endif
