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

class QOutputDev;
class KPDFPage;

//  ### COMMENTED STUFF WILL COME BACK SOON - EROS  ###
// FIXME COMMENTED STUFF WILL COME BACK SOON - EROS ###
//  ### COMMENTED STUFF WILL COME BACK SOON - EROS  ###

namespace KPDF
{

class PageWidget : public QScrollView, public KPDFDocumentObserver
{
	Q_OBJECT

public:
	PageWidget( QWidget *parent, KPDFDocument *document );
	~PageWidget();

	// create actions that interact with this widget
	void setupActions( KActionCollection * collection, KConfigGroup * config );
	void saveSettings( KConfigGroup * config );

	// inherited from KPDFDocumentObserver
	void pageSetup( const QValueList<int> & pages );
	void pageSetCurrent( int pageNumber, float position );
	void notifyPixmapChanged( int pageNumber );

        bool find(Unicode *u, int len, bool next);

    protected:
        void contentsMousePressEvent(QMouseEvent*);
        void contentsMouseReleaseEvent(QMouseEvent*);
        void contentsMouseMoveEvent(QMouseEvent*);
        virtual void keyPressEvent( QKeyEvent* );
        virtual void wheelEvent( QWheelEvent * );
        virtual void dragEnterEvent( QDragEnterEvent* );
        virtual void dropEvent( QDropEvent* );
        virtual void drawContents ( QPainter *p, int, int, int, int );

public slots:
	// connected to local actions
	void slotZoom( const QString& );
	void slotZoomIn();
	void slotZoomOut();
	void slotFitToWidthToggled( bool );
	void slotToggleScrollBars( bool on );

signals:
	void rightClick();
	void urlDropped( const KURL& );

private:
	// the document, current page and pages indices vector
	KPDFDocument * m_document;
	// FIXME only for knowing the order of pages.. not useful, change me
	QValueVector<int> m_pages;
	int m_vectorIndex;
	const KPDFPage * m_page;

	enum MouseMode { MouseNormal, MouseSelection, MouseEditing } m_mouseMode;
	// mouse related vars ...
	//enum ViewMode { ... } m_viewMode;
	//int m_viewNumber;

	// zoom related
	enum ZoomMode { FitInWindow, FitWidth, FitVisible, FixedFactor } m_zoomMode;
	float m_zoomFactor;

	// actions
	KSelectAction *m_aZoom;
	KToggleAction *m_aZoomFitWidth;


        // FIXME what atTop() means if I see 4 tiled pages on screen ?
        bool atTop()    const;
        bool atBottom() const;
        void scrollUp();
        void scrollDown();
        void updatePixmap();

        float   m_ppp; // Pixels per point
        QPoint   m_dragGrabPos;
        //double m_xMin, m_yMin, m_xMax, m_yMax;
};

}

#endif

// vim:ts=2:sw=2:tw=78:et
