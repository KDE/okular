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

#include <qlist.h>
#include <qpixmap.h>
#include <qstringlist.h>
#include <qwidget.h>
#include "core/area.h"
#include "core/observer.h"
#include "core/pagetransition.h"

class QLineEdit;
class QToolBar;
class QTimer;
class KActionCollection;
class KSelectAction;
class SmoothPathEngine;
struct PresentationFrame;
class PresentationSearchBar;

namespace Okular {
class Action;
class Annotation;
class Document;
class MovieAction;
class Page;
class RenditionAction;
}

/**
 * @short A widget that shows pages as fullscreen slides (with transitions fx).
 *
 * This is a fullscreen widget that displays 
 */
class PresentationWidget : public QWidget, public Okular::DocumentObserver
{
    Q_OBJECT
    public:
        PresentationWidget( QWidget * parent, Okular::Document * doc, KActionCollection * collection );
        ~PresentationWidget();

        // inherited from DocumentObserver
        void notifySetup( const QVector< Okular::Page * > & pages, int setupFlags );
        void notifyViewportChanged( bool smoothMove );
        void notifyPageChanged( int pageNumber, int changedFlags );
        bool canUnloadPixmap( int pageNumber ) const;
        void notifyCurrentPageChanged( int previous, int current );

    public slots:
        void slotFind();

    protected:
        // widget events
        bool event( QEvent * e );
        void keyPressEvent( QKeyEvent * e );
        void wheelEvent( QWheelEvent * e );
        void mousePressEvent( QMouseEvent * e );
        void mouseReleaseEvent( QMouseEvent * e );
        void mouseMoveEvent( QMouseEvent * e );
        void paintEvent( QPaintEvent * e );
        void resizeEvent( QResizeEvent * e );
        void leaveEvent( QEvent * e );

    private:
        const void * getObjectRect( Okular::ObjectRect::ObjectType type, int x, int y, QRect * geometry = 0 ) const;
        const Okular::Action * getLink( int x, int y, QRect * geometry = 0 ) const;
        const Okular::Annotation * getAnnotation( int x, int y, QRect * geometry = 0 ) const;
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
        void startAutoChangeTimer();
        void recalcGeometry();
        void repositionContent();
        void requestPixmaps();
        void setScreen( int );
        void applyNewScreenSize( const QSize & oldSize );
        void inhibitPowerManagement();
        void allowPowerManagement();
        void showTopBar( bool );
        // create actions that interact with this widget
        void setupActions();
        void setPlayPauseIcon();

        // cache stuff
        int m_width;
        int m_height;
        QPixmap m_lastRenderedPixmap;
        QPixmap m_lastRenderedOverlay;
        QRect m_overlayGeometry;
        const Okular::Action * m_pressedLink;
        bool m_handCursor;
        SmoothPathEngine * m_drawingEngine;
        QRect m_drawingRect;
        int m_screen;
        int m_screenInhibitCookie;
        int m_sleepInhibitCookie;

        // transition related
        QTimer * m_transitionTimer;
        QTimer * m_overlayHideTimer;
        QTimer * m_nextPageTimer;
        int m_transitionDelay;
        int m_transitionMul;
        QList< QRect > m_transitionRects;

        // misc stuff
        QWidget * m_parentWidget;
        Okular::Document * m_document;
        QVector< PresentationFrame * > m_frames;
        int m_frameIndex;
        QStringList m_metaStrings;
        QToolBar * m_topBar;
        QLineEdit *m_pagesEdit;
        PresentationSearchBar *m_searchBar;
        KActionCollection * m_ac;
        KSelectAction * m_screenSelect;
        bool m_isSetup;
        bool m_blockNotifications;
        bool m_inBlackScreenMode;
        bool m_showSummaryView;
        bool m_advanceSlides;

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
        void screenResized( int );
        void chooseScreen( QAction * );
        void toggleBlackScreenMode( bool );
        void slotProcessMovieAction( const Okular::MovieAction *action );
        void slotProcessRenditionAction( const Okular::RenditionAction *action );
        void slotTogglePlayPause();
};

#endif
