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

#include <qpixmap.h>
#include <qwidget.h>
#include <qscrollview.h>

#include <kurl.h>

#include "CharTypes.h"

class QMutex;

class LinkAction;
class PDFDoc;

class QOutputDevPixmap;

namespace KPDF
{
    /**
     * Widget displaying a pixmap containing a PDF page and Links.
     */
    class PageWidget : public QScrollView
    {
        Q_OBJECT

        enum ZoomMode { FitInWindow, FitWidth, FitVisible, FixedFactor };

    public:
        PageWidget(QWidget* parent, const char* name, QMutex *docMutex);
        ~PageWidget();
        void setPDFDocument(PDFDoc*);
        void setPixelsPerPoint(float);
        /* void setLinks(); */

        void setPage(int pagenum);
        void enableScrollBars( bool b );
        /**
         * Return true if the top resp. bottom of the page is visible.
         */
        bool atTop()    const;
        bool atBottom() const;
        void zoomTo( double _value );

        bool find(Unicode *u, int len, bool next);

    public slots:
        void zoomIn();
        void zoomOut();

        void updatePixmap();
        void scrollUp();
        void scrollDown();
        void scrollRight();
        void scrollLeft();
        void scrollBottom();
        void scrollTop();
        bool readUp();
        bool readDown();
    signals:
        void linkClicked(LinkAction*);
        void ReadUp();
        void ReadDown();
        void ZoomOut();
        void ZoomIn();
        void rightClick();
        void urlDropped( const KURL& );
        void spacePressed();
    protected:
        virtual void keyPressEvent( QKeyEvent* );
        void contentsMousePressEvent(QMouseEvent*);
        void contentsMouseReleaseEvent(QMouseEvent*);
        void contentsMouseMoveEvent(QMouseEvent*);
        virtual void wheelEvent( QWheelEvent * );
        virtual void drawContents ( QPainter *p, int, int, int, int );
        virtual void dragEnterEvent( QDragEnterEvent* );
        virtual void dropEvent( QDropEvent* );
    private:

        QOutputDevPixmap * m_outputdev;
        PDFDoc* m_doc;
        QMutex* m_docMutex;
        float   m_ppp; // Pixels per point
        float		m_zoomFactor;
        ZoomMode m_zoomMode;

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
