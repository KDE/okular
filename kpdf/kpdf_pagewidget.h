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
#include "document.h"

#include "CharTypes.h"

class KURL;
class KActionCollection;
class KConfigGroup;

class LinkAction;
class PDFDoc;

class QOutputDev;
class KPDFDocument;

namespace KPDF
{
    /**
     * Widget displaying a pixmap containing a PDF page and Links.
     */
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

        // FIXME what atTop() means if I see 4 tiled pages on screen ?
        bool atTop()    const;
        bool atBottom() const;

        bool find(Unicode *u, int len, bool next);
        /* void setLinks(); */

    public slots:
		void slotScrollUp();
		void slotScrollDown();

		// connected to local actions
		void slotZoom( const QString& );
		void slotZoomIn();
		void slotZoomOut();
		void slotFitToWidthToggled( bool );
		void slotToggleScrollBars( bool on );

private:
	// the document
	KPDFDocument * m_document;

	// zoom related
	enum ZoomMode { FitInWindow, FitWidth, FitVisible, FixedFactor } m_zoomMode;
	float m_zoomFactor;

	// actions
	KSelectAction *m_aZoom;
	KToggleAction *m_aZoomFitWidth;

    protected:
        virtual void keyPressEvent( QKeyEvent* );
        void contentsMousePressEvent(QMouseEvent*);
        void contentsMouseReleaseEvent(QMouseEvent*);
        void contentsMouseMoveEvent(QMouseEvent*);
        virtual void wheelEvent( QWheelEvent * );
        virtual void drawContents ( QPainter *p, int, int, int, int );
        virtual void dragEnterEvent( QDragEnterEvent* );
        virtual void dropEvent( QDropEvent* );

signals:
    void zoomChanged( float newZoom );

        void linkClicked(LinkAction*);
        void rightClick();
        void urlDropped( const KURL& );

    private:
        void updatePixmap();

        QOutputDev *m_outputdev;
        PDFDoc* m_doc;
        float   m_ppp; // Pixels per point

        // first page is page 1
        int m_currentPage;
        QPoint   m_dragGrabPos;
        LinkAction* m_pressedAction;

        bool m_selection;
        double m_xMin, m_yMin, m_xMax, m_yMax;
    };
}

#endif

// vim:ts=2:sw=2:tw=78:et
