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

#ifndef _KPDF_PAGEVIEW_H_
#define _KPDF_PAGEVIEW_H_

#include <qscrollview.h>
#include <qvaluevector.h>

#include "CharTypes.h"
#include "document.h"

class KURL;
class KActionCollection;
class KConfigGroup;

class PageWidget;
class PageViewPrivate;

/**
 * @short A scrollview that displays page pixmaps.
 *
 */
class PageView : public QScrollView, public KPDFDocumentObserver
{
Q_OBJECT

public:
    PageView( QWidget *parent, KPDFDocument *document );
    ~PageView();

    // Zoom mode ( last 2 are internally used only! )
    enum ZoomMode { ZoomFixed, ZoomFitWidth, ZoomFitPage, ZoomFitText,  ZoomIn, ZoomOut };
    enum MouseMode { MouseNormal, MouseSelection, MouseEdit };

    // create actions that interact with this widget
    void setupActions( KActionCollection * collection, KConfigGroup * config );
    void saveSettings( KConfigGroup * config );

    // inherited from KPDFDocumentObserver
    uint observerId() { return PAGEVIEW_ID; }
    void pageSetup( const QValueVector<KPDFPage*> & pages, bool documentChanged );
    void pageSetCurrent( int pageNumber, float position );
    void notifyPixmapChanged( int pageNumber );

protected:
    void contentsMousePressEvent( QMouseEvent* );
    void contentsMouseReleaseEvent( QMouseEvent* );
    void contentsMouseMoveEvent( QMouseEvent* );

    void keyPressEvent( QKeyEvent* );
    void wheelEvent( QWheelEvent* );
    void viewportResizeEvent( QResizeEvent* );

    void dragEnterEvent( QDragEnterEvent* );
    void dropEvent( QDropEvent* );

signals:
    void urlDropped( const KURL& );
    void rightClick();

private slots:
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
    void slotSetMouseSelect();
    void slotSetMouseDraw();
    void slotScrollUp();
    void slotScrollDown();
    void slotToggleScrollBars( bool on );

    // activated either directly or via QTimer on the viewportResizeEvent
    void slotRelayoutPages();
    // activated either directly or via the contentsMoving(int,int) signal
    void slotRequestVisiblePixmaps( int left = -1, int top = -1 );
    // activated by the autoscroll timer (Shift+Up/Down keys)
    void slotAutoScoll();

private:
    // update internal zoom values and end in a slotRelayoutPages();
    void updateZoom( ZoomMode newZm );
    // update the text on the label using global zoom value or current page's one
    void updateZoomText();
    // return the widget placed on a certain point or 0 if clicking on empty space
    PageWidget * pickPageOnPoint( int x, int y );

    // FIXME REMOVE ME what does atTop() means if I see 4 tiled pages on screen ?
    bool atTop()    const;
    bool atBottom() const;
    void scrollUp();
    void scrollDown();

    // don't want to expose classes in here
    class PageViewPrivate * d;
};

#endif

// vim:ts=2:sw=2:tw=78:et
