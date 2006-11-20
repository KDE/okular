/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_PRESENTATIONWIDGET_H_
#define _OKULAR_PRESENTATIONWIDGET_H_

#include <qdialog.h>
#include <qlist.h>
#include <qpixmap.h>
#include <qstringlist.h>
#include "core/observer.h"
#include "core/pagetransition.h"

class QLineEdit;
class QToolBar;
class QTimer;
class KActionCollection;
class AnnotatorEngine;
class PresentationFrame;

namespace Okular {
struct Annotation;
class Document;
class Page;
class Link;
}

/**
 * @short A widget that shows pages as fullscreen slides (with transitions fx).
 *
 * This is a fullscreen widget that displays 
 */
class PresentationWidget : public QDialog, public Okular::DocumentObserver
{
    Q_OBJECT
    public:
        PresentationWidget( QWidget * parent, Okular::Document * doc );
        ~PresentationWidget();

        // inherited from DocumentObserver
        uint observerId() const { return PRESENTATION_ID; }
        void notifySetup( const QVector< Okular::Page * > & pages, bool documentChanged );
        void notifyViewportChanged( bool smoothMove );
        void notifyPageChanged( int pageNumber, int changedFlags );
        bool canUnloadPixmap( int pageNumber ) const;

        // create actions that interact with this widget
        void setupActions( KActionCollection * collection );

    protected:
        // widget events
        bool event( QEvent * e );
        void keyPressEvent( QKeyEvent * e );
        void wheelEvent( QWheelEvent * e );
        void mousePressEvent( QMouseEvent * e );
        void mouseReleaseEvent( QMouseEvent * e );
        void mouseMoveEvent( QMouseEvent * e );
        void paintEvent( QPaintEvent * e );

    private:
        const Okular::Link * getLink( int x, int y, QRect * geometry = 0 ) const;
        void testCursorOnLink( int x, int y );
        void overlayClick( const QPoint & position );
        void changePage( int newPage );
        void generatePage( bool disableTransition = false );
        void generateIntroPage( QPainter & p );
        void generateContentsPage( int page, QPainter & p );
        void generateOverlay();
        void initTransition( const Okular::PageTransition *transition );
        const Okular::PageTransition defaultTransition() const;
        const Okular::PageTransition defaultTransition( int ) const;
        QRect routeMouseDrawingEvent( QMouseEvent * );

        // cache stuff
        int m_width;
        int m_height;
        QPixmap m_lastRenderedPixmap;
        QPixmap m_lastRenderedOverlay;
        QRect m_overlayGeometry;
        const Okular::Link * m_pressedLink;
        bool m_handCursor;
        QList< Okular::Annotation * > m_currentPageDrawings;
        AnnotatorEngine * m_drawingEngine;
        QRect m_drawingRect;

        // transition related
        QTimer * m_transitionTimer;
        QTimer * m_overlayHideTimer;
        int m_transitionDelay;
        int m_transitionMul;
        QList< QRect > m_transitionRects;

        // misc stuff
        Okular::Document * m_document;
        QVector< PresentationFrame * > m_frames;
        int m_frameIndex;
        QStringList m_metaStrings;
        QToolBar * m_topBar;
        QLineEdit *m_pagesEdit;
        KActionCollection * m_ac;

    private slots:
        void slotNextPage();
        void slotPrevPage();
        void slotFirstPage();
        void slotLastPage();
        void slotHideOverlay();
        void slotTransitionStep();
        void slotDelayedEvents();
        void slotPageChanged();
        void togglePencilMode( bool );
        void clearDrawings();
};

#endif
