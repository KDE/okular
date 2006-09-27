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

#ifndef _OKULAR_PAGEVIEW_H_
#define _OKULAR_PAGEVIEW_H_

#include <q3scrollview.h>
#include <qvector.h>
#include <qlinkedlist.h>
#include "ui/pageviewutils.h"
#include "core/observer.h"

class KActionCollection;

namespace Okular {
class Document;
struct Annotation;
}

class PageViewPrivate;

/**
 * @short The main view. Handles zoom and continuous mode.. oh, and page
 * @short display of course :-)
 * ...
 */
class PageView : public Q3ScrollView, public Okular::DocumentObserver
{
    Q_OBJECT

    public:
        PageView( QWidget *parent, Okular::Document *document );
        ~PageView();

        //set pop-up annotation window states,such as hide/open ,position...
        void setAnnotsWindow(Okular::Annotation * annot);

        // Zoom mode ( last 4 are internally used only! )
        enum ZoomMode { ZoomFixed = 0, ZoomFitWidth = 1, ZoomFitPage = 2, ZoomFitText,
                        ZoomIn, ZoomOut, ZoomRefreshCurrent };
        enum MouseMode { MouseNormal, MouseZoom, MouseSelect, MouseTextSelect };

        // create actions that interact with this widget
        void setupActions( KActionCollection * collection );

        // misc methods (from RMB menu/children)
        bool canFitPageWidth();
        void fitPageWidth( int page );
        // keep in sync with pageviewutils
        void displayMessage( const QString & message , PageViewMessage::Icon icon=PageViewMessage::Info, int duration=-1 );

        // inherited from DocumentObserver
        uint observerId() const { return PAGEVIEW_ID; }
        void notifySetup( const QVector< Okular::Page * > & pages, bool documentChanged );
        void notifyViewportChanged( bool smoothMove );
        void notifyPageChanged( int pageNumber, int changedFlags );
        void notifyContentsCleared( int changedFlags );
        bool canUnloadPixmap( int pageNum );

    public slots:
        void errorMessage( const QString & message, int duration )
        {
            displayMessage( message, PageViewMessage::Error, duration );
        }

        void noticeMessage( const QString & message, int duration )
        {
            displayMessage( message, PageViewMessage::Info, duration );
        }

        void warningMessage( const QString & message, int duration )
        {
            displayMessage( message, PageViewMessage::Warning, duration );
        }

    signals:
        void urlDropped( const KUrl& );
        void rightClick( const Okular::Page *, const QPoint & );

    protected:
        bool event( QEvent* );

        // viewport events
        void viewportPaintEvent( QPaintEvent * pe );
        void viewportResizeEvent( QResizeEvent* );

        // mouse / keyboard events
        void keyPressEvent( QKeyEvent* );
        void inputMethodEvent( QInputMethodEvent * );
        void contentsMouseMoveEvent( QMouseEvent* );
        void contentsMousePressEvent( QMouseEvent* );
        void contentsMouseReleaseEvent( QMouseEvent* );
        void wheelEvent( QWheelEvent* );

        // drag and drop related events
        void dragEnterEvent( QDragEnterEvent* );
        void dragMoveEvent( QDragMoveEvent* );
        void dropEvent( QDropEvent* );

    private:
        // draw background and items on the opened qpainter
        void drawDocumentOnPainter( const QRect & pageViewRect, QPainter * p );
        // update item width and height using current zoom parameters
        void updateItemSize( PageViewItem * item, int columnWidth, int rowHeight );
        // return the widget placed on a certain point or 0 if clicking on empty space
        PageViewItem * pickItemOnPoint( int x, int y );
        // start / modify / clear selection rectangle
        void selectionStart( const QPoint & pos, const QColor & color, bool aboveAll = false );
        void selectionEndPoint( const QPoint & pos );
        void selectionClear();
        void textSelectionForItem( PageViewItem * item, const QPoint & startPoint = QPoint(), const QPoint & endPoint = QPoint() );
        // update internal zoom values and end in a slotRelayoutPages();
        void updateZoom( ZoomMode newZm );
        // update the text on the label using global zoom value or current page's one
        void updateZoomText();
	void textSelectionClear();
        // updates cursor
        void updateCursor( const QPoint &p );
        // does the type ahead search
        void doTypeAheadSearch();

	int viewColumns();
	int viewRows();

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
        // type-ahead find timeout
        void slotStopFindAhead();
        // sow the welcome message
        void slotShowWelcome();

        // connected to local actions (toolbar, menu, ..)
        void slotZoom();
        void slotZoomIn();
        void slotZoomOut();
        void slotFitToWidthToggled( bool );
        void slotFitToPageToggled( bool );
        void slotFitToTextToggled( bool );
        void slotRenderMode( int );
        void slotContinuousToggled( bool );
        void slotSetMouseNormal();
        void slotSetMouseZoom();
        void slotSetMouseSelect();
        void slotSetMouseTextSelect();
        void slotToggleAnnotator( bool );
        void slotScrollUp();
        void slotScrollDown();
};

#endif
