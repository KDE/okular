/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   With portions of code from kpdf_pagewidget.h by:                      *
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

#include "document.h"

class KURL;
class KActionCollection;

class PageViewItem;
class PageViewPrivate;

/**
 * @short The main view. Handles zoom and continous mode.. oh, and page
 * @short display of course :-)
 * ...
 */
class PageView : public QScrollView, public KPDFDocumentObserver
{
    Q_OBJECT

    public:
        PageView( QWidget *parent, KPDFDocument *document );
        ~PageView();

        // Zoom mode ( last 4 are internally used only! )
        enum ZoomMode { ZoomFixed, ZoomFitWidth, ZoomFitPage, ZoomFitText,
                        ZoomIn, ZoomOut, ZoomRefreshCurrent };
        enum MouseMode { MouseNormal, MouseZoom, MouseSelText, MouseSelGfx, MouseEdit };

        // create actions that interact with this widget
        void setupActions( KActionCollection * collection );

        // inherited from KPDFDocumentObserver
        uint observerId() const { return PAGEVIEW_ID; }
        void pageSetup( const QValueVector<KPDFPage*> & pages, bool documentChanged );
        void pageSetCurrent( int pageNumber, const QRect & viewport );
        void notifyPixmapChanged( int pageNumber );

    signals:
        void urlDropped( const KURL& );
        void rightClick();

    protected:
        // main draw loop, draws pageViews on viewport
        void viewportPaintEvent( QPaintEvent * pe );
        void viewportResizeEvent( QResizeEvent* );

        // mouse / keyboard events
        void keyPressEvent( QKeyEvent* );
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
        void selectionStart( int x, int y, bool aboveAll = false, PageViewItem * pageLock = 0 );
        void selectionEndPoint( int x, int y );
        void selectionClear();
        // update internal zoom values and end in a slotRelayoutPages();
        void updateZoom( ZoomMode newZm );
        // update the text on the label using global zoom value or current page's one
        void updateZoomText();

        // don't want to expose classes in here
        class PageViewPrivate * d;

    private slots:
        // activated either directly or via QTimer on the viewportResizeEvent
        void slotRelayoutPages();
        // activated either directly or via the contentsMoving(int,int) signal
        void slotRequestVisiblePixmaps( int left = -1, int top = -1 );
        // activated by the autoscroll timer (Shift+Up/Down keys)
        void slotAutoScoll();

        // connected to local actions
        void slotZoom();
        void slotZoomIn();
        void slotZoomOut();
        void slotFitToWidthToggled( bool );
        void slotFitToPageToggled( bool );
        void slotFitToTextToggled( bool );
        void slotTwoPagesToggled( bool );
        void slotContinousToggled( bool );
        void slotSetMouseNormal();
        void slotSetMouseZoom();
        void slotSetMouseSelText();
        void slotSetMouseSelGfx();
        void slotSetMouseDraw();
        void slotScrollUp();
        void slotScrollDown();
};

#endif
