/***************************************************************************
 *   Copyright (C) 2002 by Wilco Greven <greven@kde.org>                   *
 *   Copyright (C) 2003 by Christophe Devriese                             *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Laurent Montel <montel@kde.org>                 *
 *   Copyright (C) 2003 by Kurt Pfeifle <kpfeifle@danka.de>                *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
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

    // some enums used to control view behavior
    enum ViewMode { ViewSingle, ViewDouble, ViewContinous };
    enum ZoomMode { ZoomFixed, ZoomFitWidth, ZoomFitPage, ZoomFitText };
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

	void viewportResizeEvent( QResizeEvent * );
	void keyPressEvent( QKeyEvent* );
	void wheelEvent( QWheelEvent * );

	void dragEnterEvent( QDragEnterEvent* );
	void dropEvent( QDropEvent* );

private slots:
	// connected to local actions
	void slotZoom( const QString& );
	void slotZoomIn();
	void slotZoomOut();
	void slotFitToWidthToggled( bool );
    void slotFitToPageToggled( bool );
    void slotFitToTextToggled( bool );

	void slotSetViewSingle();
	void slotSetViewDouble();
	void slotSetViewContinous();

	void slotSetMouseNormal();
	void slotSetMouseSelect();
	void slotSetMouseDraw();

	void slotToggleScrollBars( bool on );
	// activated directly or via QTimer on the viewportResizeEvent
	void slotUpdateView( bool repaint = true );

signals:
	void urlDropped( const KURL& );
	void rightClick();

private:
    void reLayoutPages();
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
