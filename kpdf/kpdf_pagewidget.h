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

#ifndef _KPDF_PAGEWIDGET_H_
#define _KPDF_PAGEWIDGET_H_

#include <qscrollview.h>
#include <qvaluevector.h>

#include "CharTypes.h"
#include "document.h"

class KURL;
class KActionCollection;
class KConfigGroup;


class PageWidget : public QScrollView, public KPDFDocumentObserver
{
	Q_OBJECT

public:
	PageWidget( QWidget *parent, KPDFDocument *document );

	// create actions that interact with this widget
	void setupActions( KActionCollection * collection, KConfigGroup * config );
	void saveSettings( KConfigGroup * config );

	// inherited from KPDFDocumentObserver
	void pageSetup( const QValueList<int> & pages );
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
	void drawContents( QPainter *p, int, int, int, int );

private slots:
	// connected to local actions
	void slotZoom( const QString& );
	void slotZoomIn();
	void slotZoomOut();
	void slotFitToWidthToggled( bool );
	void slotToggleScrollBars( bool on );
	// activated directly or via QTimer on the viewportResizeEvent
	void slotUpdateView( bool forceRepaint = false );

signals:
	void urlDropped( const KURL& );
	void rightClick();

private:
	// FIXME what does atTop() means if I see 4 tiled pages on screen ?
	bool atTop()    const;
	bool atBottom() const;
	void scrollUp();
	void scrollDown();

	// the document, current page and pages indices vector
	KPDFDocument * m_document;
	// FIXME only for knowing the order of pages.. not useful, change me
	QValueVector<int> m_pages;
	int m_vectorIndex;
	const KPDFPage * m_page;
	int m_pageWidth;
	int m_pageHeight;

	enum MouseMode { MouseNormal, MouseSelection /*MouseEditing*/ } m_mouseMode;
	QPoint m_mouseStartPos;
	// mouse related vars ...
	//enum ViewMode { ... } m_viewMode;
	//int m_viewNumber;

	// zoom related
	enum ZoomMode { FixedFactor, FitWidth, FitPage /*FitVisible*/ } m_zoomMode;
	float m_zoomFactor;

	// actions
	KSelectAction *m_aZoom;
	KToggleAction *m_aZoomFitWidth;

	QTimer *m_delayTimer;
};

#endif

// vim:ts=2:sw=2:tw=78:et
