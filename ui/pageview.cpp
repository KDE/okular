/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2004-2006 by Albert Astals Cid <tsdgeos@terra.es>       *
 *                                                                         *
 *   With portions of code from kpdf/kpdf_pagewidget.cc by:                *
 *     Copyright (C) 2002 by Wilco Greven <greven@kde.org>                 *
 *     Copyright (C) 2003 by Christophe Devriese                           *
 *                           <Christophe.Devriese@student.kuleuven.ac.be>  *
 *     Copyright (C) 2003 by Laurent Montel <montel@kde.org>               *
 *     Copyright (C) 2003 by Dirk Mueller <mueller@kde.org>                *
 *     Copyright (C) 2004 by James Ots <kde@jamesots.com>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qcursor.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <dcopclient.h>
#include <kcursor.h>
#include <kiconloader.h>
#include <kurldrag.h>
#include <kaction.h>
#include <kstdaccel.h>
#include <kactioncollection.h>
#include <kpopupmenu.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kimageeffect.h>
#include <kimageio.h>
#include <kapplication.h>
#include <kdebug.h>

// system includes
#include <math.h>
#include <stdlib.h>

// local includes
#include "pageview.h"
#include "pageviewutils.h"
#include "pagepainter.h"
#include "core/document.h"
#include "core/page.h"
#include "core/link.h"
#include "core/generator.h"
#include "conf/settings.h"

#define ROUND(x) (int(x + 0.5))

// definition of searchID for this class
#define PAGEVIEW_SEARCH_ID 2

// structure used internally by PageView for data storage
class PageViewPrivate
{
public:
    // the document, pageviewItems and the 'visible cache'
    KPDFDocument * document;
    QValueVector< PageViewItem * > items;
    QValueList< PageViewItem * > visibleItems;

    // view layout (columns and continuous in Settings), zoom and mouse
    PageView::ZoomMode zoomMode;
    float zoomFactor;
    PageView::MouseMode mouseMode;
    QPoint mouseGrabPos;
    QPoint mousePressPos;
    int mouseMidStartY;
    bool mouseOnRect;
    QRect mouseSelectionRect;
    QColor selectionRectColor;

    // type ahead find
    bool typeAheadActive;
    QString typeAheadString;
    QTimer * findTimeoutTimer;
    // viewport move
    bool viewportMoveActive;
    QTime viewportMoveTime;
    QPoint viewportMoveDest;
    QTimer * viewportMoveTimer;
    // auto scroll
    int scrollIncrement;
    QTimer * autoScrollTimer;
    // other stuff
    QTimer * delayResizeTimer;
    bool dirtyLayout;
    bool blockViewport;                 // prevents changes to viewport
    bool blockPixmapsRequest;           // prevent pixmap requests
    PageViewMessage * messageWindow;    // in pageviewutils.h
    PageViewTip * tip;

    // drag scroll
    QPoint dragScrollVector;
    QTimer dragScrollTimer;

    // actions
    KToggleAction * aMouseNormal;
    KToggleAction * aMouseSelect;
    KToggleAction * aMouseEdit;
    KSelectAction * aZoom;
    KToggleAction * aZoomFitWidth;
    KToggleAction * aZoomFitPage;
    KToggleAction * aZoomFitText;
    KToggleAction * aViewTwoPages;
    KToggleAction * aViewContinuous;
    KAction * aPrevAction;
};



class PageViewTip : public QToolTip
{
    public:
        PageViewTip( PageView * view )
            : QToolTip( view->viewport() ), m_view( view )
        {
        }

        ~PageViewTip()
        {
            remove( m_view->viewport() );
        }


    protected:
        void maybeTip( const QPoint &p );

    private:
        PageView * m_view;
};

void PageViewTip::maybeTip( const QPoint &_p )
{
    QPoint p( _p.x() + m_view->contentsX(), _p.y() + m_view->contentsY() );
    PageViewItem * pageItem = m_view->pickItemOnPoint( p.x(), p.y() );
    if ( pageItem && m_view->d->mouseMode == PageView::MouseNormal )
    {
        double nX = (double)(p.x() - pageItem->geometry().left()) / (double)pageItem->width(),
               nY = (double)(p.y() - pageItem->geometry().top()) / (double)pageItem->height();

        // if over a ObjectRect (of type Link) change cursor to hand
        const ObjectRect * object = pageItem->page()->hasObject( ObjectRect::Link, nX, nY );
        if ( object )
        {
            // set tooltip over link's rect
            KPDFLink *link = (KPDFLink *)object->pointer();
            QString strtip = link->linkTip();
            if ( !strtip.isEmpty() )
            {
                QRect linkRect = object->geometry( pageItem->width(), pageItem->height() );
                linkRect.moveBy( - m_view->contentsX() + pageItem->geometry().left(), - m_view->contentsY() + pageItem->geometry().top() );
                tip( linkRect, strtip );
            }
        }
    }
}



/* PageView. What's in this file? -> quick overview.
 * Code weight (in rows) and meaning:
 *  160 - constructor and creating actions plus their connected slots (empty stuff)
 *  70  - DocumentObserver inherited methodes (important)
 *  550 - events: mouse, keyboard, drag/drop
 *  170 - slotRelayoutPages: set contents of the scrollview on continuous/single modes
 *  100 - zoom: zooming pages in different ways, keeping update the toolbar actions, etc..
 *  other misc functions: only slotRequestVisiblePixmaps and pickItemOnPoint noticeable,
 * and many insignificant stuff like this comment :-)
 */
PageView::PageView( QWidget *parent, KPDFDocument *document )
    : QScrollView( parent, "KPDF::pageView", WStaticContents | WNoAutoErase )
{
    // create and initialize private storage structure
    d = new PageViewPrivate();
    d->document = document;
    d->zoomMode = (PageView::ZoomMode)KpdfSettings::zoomMode();
    d->zoomFactor = KpdfSettings::zoomFactor();
    d->mouseMode = MouseNormal;
    d->mouseMidStartY = -1;
    d->mouseOnRect = false;
    d->typeAheadActive = false;
    d->findTimeoutTimer = 0;
    d->viewportMoveActive = false;
    d->viewportMoveTimer = 0;
    d->scrollIncrement = 0;
    d->autoScrollTimer = 0;
    d->delayResizeTimer = 0;
    d->dirtyLayout = false;
    d->blockViewport = false;
    d->blockPixmapsRequest = false;
    d->messageWindow = new PageViewMessage(this);
    d->tip = new PageViewTip( this );
    d->aPrevAction = 0;

    // widget setup: setup focus, accept drops and track mouse
    viewport()->setFocusProxy( this );
    viewport()->setFocusPolicy( StrongFocus );
    //viewport()->setPaletteBackgroundColor( Qt::white );
    viewport()->setBackgroundMode( Qt::NoBackground );
    setResizePolicy( Manual );
    setAcceptDrops( true );
    setDragAutoScroll( false );
    viewport()->setMouseTracking( true );

    // conntect the padding of the viewport to pixmaps requests
    connect( this, SIGNAL(contentsMoving(int, int)), this, SLOT(slotRequestVisiblePixmaps(int, int)) );
    connect( &d->dragScrollTimer, SIGNAL(timeout()), this, SLOT(slotDragScroll()) );

    // set a corner button to resize the view to the page size
//    QPushButton * resizeButton = new QPushButton( viewport() );
//    resizeButton->setPixmap( SmallIcon("crop") );
//    setCornerWidget( resizeButton );
//    resizeButton->setEnabled( false );
    // connect(...);
    setInputMethodEnabled( true );

    // schedule the welcome message
    QTimer::singleShot( 0, this, SLOT( slotShowWelcome() ) );
}

PageView::~PageView()
{
    // delete all widgets
    QValueVector< PageViewItem * >::iterator dIt = d->items.begin(), dEnd = d->items.end();
    for ( ; dIt != dEnd; ++dIt )
        delete *dIt;
    delete d->tip;
    d->tip = 0;
    d->document->removeObserver( this );
    delete d;
}

void PageView::setupActions( KActionCollection * ac )
{
    // Zoom actions ( higher scales takes lots of memory! )
    d->aZoom = new KSelectAction( i18n( "Zoom" ), "viewmag", 0, this, SLOT( slotZoom() ), ac, "zoom_to" );
    d->aZoom->setEditable( true );
#if KDE_IS_VERSION(3,4,89)
    d->aZoom->setMaxComboViewCount( 13 );
#endif
    updateZoomText();

    KStdAction::zoomIn( this, SLOT( slotZoomIn() ), ac, "zoom_in" );

    KStdAction::zoomOut( this, SLOT( slotZoomOut() ), ac, "zoom_out" );

    d->aZoomFitWidth = new KToggleAction( i18n("Fit to Page &Width"), "view_fit_width", 0, ac, "zoom_fit_width" );
    connect( d->aZoomFitWidth, SIGNAL( toggled( bool ) ), SLOT( slotFitToWidthToggled( bool ) ) );

    d->aZoomFitPage = new KToggleAction( i18n("Fit to &Page"), "view_fit_window", 0, ac, "zoom_fit_page" );
    connect( d->aZoomFitPage, SIGNAL( toggled( bool ) ), SLOT( slotFitToPageToggled( bool ) ) );

    d->aZoomFitText = new KToggleAction( i18n("Fit to &Text"), "viewmagfit", 0, ac, "zoom_fit_text" );
    connect( d->aZoomFitText, SIGNAL( toggled( bool ) ), SLOT( slotFitToTextToggled( bool ) ) );

    // View-Layout actions
    d->aViewTwoPages = new KToggleAction( i18n("&Two Pages"), "view_left_right", 0, ac, "view_twopages" );
    connect( d->aViewTwoPages, SIGNAL( toggled( bool ) ), SLOT( slotTwoPagesToggled( bool ) ) );
    d->aViewTwoPages->setChecked( KpdfSettings::viewColumns() > 1 );

    d->aViewContinuous = new KToggleAction( i18n("&Continuous"), "view_text", 0, ac, "view_continuous" );
    connect( d->aViewContinuous, SIGNAL( toggled( bool ) ), SLOT( slotContinuousToggled( bool ) ) );
    d->aViewContinuous->setChecked( KpdfSettings::viewContinuous() );

    // Mouse-Mode actions
    d->aMouseNormal = new KRadioAction( i18n("&Browse Tool"), "mouse", 0, this, SLOT( slotSetMouseNormal() ), ac, "mouse_drag" );
    d->aMouseNormal->setExclusiveGroup( "MouseType" );
    d->aMouseNormal->setChecked( true );

    KToggleAction * mz = new KRadioAction( i18n("&Zoom Tool"), "viewmag", 0, this, SLOT( slotSetMouseZoom() ), ac, "mouse_zoom" );
    mz->setExclusiveGroup( "MouseType" );

    d->aMouseSelect = new KRadioAction( i18n("&Select Tool"), "frame_edit", 0, this, SLOT( slotSetMouseSelect() ), ac, "mouse_select" );
    d->aMouseSelect->setExclusiveGroup( "MouseType" );

/*    d->aMouseEdit = new KRadioAction( i18n("Draw"), "edit", 0, this, SLOT( slotSetMouseDraw() ), ac, "mouse_draw" );
    d->aMouseEdit->setExclusiveGroup("MouseType");
    d->aMouseEdit->setEnabled( false ); // implement feature before removing this line*/

    // Other actions
    KAction * su = new KAction( i18n("Scroll Up"), 0, this, SLOT( slotScrollUp() ), ac, "view_scroll_up" );
    su->setShortcut( "Shift+Up" );

    KAction * sd = new KAction( i18n("Scroll Down"), 0, this, SLOT( slotScrollDown() ), ac, "view_scroll_down" );
    sd->setShortcut( "Shift+Down" );
}

bool PageView::canFitPageWidth()
{
    return d->zoomMode != ZoomFitWidth;
}

void PageView::fitPageWidth( int /*page*/ )
{
    d->aZoom->setCurrentItem(0);
    slotZoom();
}

//BEGIN DocumentObserver inherited methods
void PageView::notifySetup( const QValueVector< KPDFPage * > & pageSet, bool documentChanged )
{
    // reuse current pages if nothing new
    if ( ( pageSet.count() == d->items.count() ) && !documentChanged )
    {
        int count = pageSet.count();
        for ( int i = 0; (i < count) && !documentChanged; i++ )
            if ( (int)pageSet[i]->number() != d->items[i]->pageNumber() )
                documentChanged = true;
        if ( !documentChanged )
            return;
    }

    // delete all widgets (one for each page in pageSet)
    QValueVector< PageViewItem * >::iterator dIt = d->items.begin(), dEnd = d->items.end();
    for ( ; dIt != dEnd; ++dIt )
        delete *dIt;
    d->items.clear();
    d->visibleItems.clear();

    // create children widgets
    QValueVector< KPDFPage * >::const_iterator setIt = pageSet.begin(), setEnd = pageSet.end();
    for ( ; setIt != setEnd; ++setIt )
        d->items.push_back( new PageViewItem( *setIt ) );

    if ( pageSet.count() > 0 )
        // TODO for Enrico: Check if doing always the slotRelayoutPages() is not
        // suboptimal in some cases, i'd say it is not but a recheck will not hurt
        // Need slotRelayoutPages() here instead of d->dirtyLayout = true
        // because opening a pdf from another pdf will not trigger a viewportchange
        // so pages are never relayouted
        QTimer::singleShot(0, this, SLOT(slotRelayoutPages()));
    else
    {
        // update the mouse cursor when closing because we may have close through a link and
        // want the cursor to come back to the normal cursor
        updateCursor( viewportToContents( mapFromGlobal( QCursor::pos() ) ) );
        resizeContents( 0, 0 );
    }

    // OSD to display pages
    if ( documentChanged && pageSet.count() > 0 && KpdfSettings::showOSD() )
        d->messageWindow->display(
            i18n(" Loaded a one-page document.",
                 " Loaded a %n-page document.",
                 pageSet.count() ),
            PageViewMessage::Info, 4000 );
}

void PageView::notifyViewportChanged( bool smoothMove )
{
    // if we are the one changing viewport, skip this nofity
    if ( d->blockViewport )
        return;

    // block setViewport outgoing calls
    d->blockViewport = true;

    // find PageViewItem matching the viewport description
    const DocumentViewport & vp = d->document->viewport();
    PageViewItem * item = 0;
    QValueVector< PageViewItem * >::iterator iIt = d->items.begin(), iEnd = d->items.end();
    for ( ; iIt != iEnd; ++iIt )
        if ( (*iIt)->pageNumber() == vp.pageNumber )
        {
            item = *iIt;
            break;
        }
    if ( !item )
    {
        kdDebug() << "viewport has no matching item!" << endl;
        d->blockViewport = false;
        return;
    }

    // relayout in "Single Pages" mode or if a relayout is pending
    d->blockPixmapsRequest = true;
    if ( !KpdfSettings::viewContinuous() || d->dirtyLayout )
        slotRelayoutPages();

    // restore viewport center or use default {x-center,v-top} alignment
    const QRect & r = item->geometry();
    int newCenterX = r.left(),
        newCenterY = r.top();
    if ( vp.rePos.enabled )
    {
        if (vp.rePos.pos == DocumentViewport::Center)
        {
            newCenterX += (int)( vp.rePos.normalizedX * (double)r.width() );
            newCenterY += (int)( vp.rePos.normalizedY * (double)r.height() );
        }
        else
        {
            // TopLeft
            newCenterX += (int)( vp.rePos.normalizedX * (double)r.width() + viewport()->width() / 2 );
            newCenterY += (int)( vp.rePos.normalizedY * (double)r.height() + viewport()->height() / 2 );
        }
    }
    else
    {
        newCenterX += r.width() / 2;
        newCenterY += visibleHeight() / 2 - 10;
    }

    // if smooth movement requested, setup parameters and start it
    if ( smoothMove )
    {
        d->viewportMoveActive = true;
        d->viewportMoveTime.start();
        d->viewportMoveDest.setX( newCenterX );
        d->viewportMoveDest.setY( newCenterY );
        if ( !d->viewportMoveTimer )
        {
            d->viewportMoveTimer = new QTimer( this );
            connect( d->viewportMoveTimer, SIGNAL( timeout() ),
                     this, SLOT( slotMoveViewport() ) );
        }
        d->viewportMoveTimer->start( 25 );
        verticalScrollBar()->setEnabled( false );
        horizontalScrollBar()->setEnabled( false );
    }
    else
        center( newCenterX, newCenterY );
    d->blockPixmapsRequest = false;

    // request visible pixmaps in the current viewport and recompute it
    slotRequestVisiblePixmaps();

    // enable setViewport calls
    d->blockViewport = false;

    // update zoom text if in a ZoomFit/* zoom mode
    if ( d->zoomMode != ZoomFixed )
        updateZoomText();

    // since the page has moved below cursor, update it
    updateCursor( viewportToContents( mapFromGlobal( QCursor::pos() ) ) );
}

void PageView::notifyPageChanged( int pageNumber, int changedFlags )
{
    // only handle pixmap / highlight changes notifies
    if ( changedFlags & DocumentObserver::Bookmark )
        return;

    // iterate over visible items: if page(pageNumber) is one of them, repaint it
    QValueList< PageViewItem * >::iterator iIt = d->visibleItems.begin(), iEnd = d->visibleItems.end();
    for ( ; iIt != iEnd; ++iIt )
        if ( (*iIt)->pageNumber() == pageNumber )
        {
            // update item's rectangle plus the little outline
            QRect expandedRect = (*iIt)->geometry();
            expandedRect.addCoords( -1, -1, 3, 3 );
            updateContents( expandedRect );

            // if we were "zoom-dragging" do not overwrite the "zoom-drag" cursor
            if ( cursor().shape() != Qt::SizeVerCursor )
            {
                // since the page has been regenerated below cursor, update it
                updateCursor( viewportToContents( mapFromGlobal( QCursor::pos() ) ) );
            }
            break;
        }
}

void PageView::notifyContentsCleared( int changedFlags )
{
    // if pixmaps were cleared, re-ask them
    if ( changedFlags & DocumentObserver::Pixmap )
        slotRequestVisiblePixmaps();
}

bool PageView::canUnloadPixmap( int pageNumber )
{
    // if the item is visible, forbid unloading
    QValueList< PageViewItem * >::iterator vIt = d->visibleItems.begin(), vEnd = d->visibleItems.end();
    for ( ; vIt != vEnd; ++vIt )
        if ( (*vIt)->pageNumber() == pageNumber )
            return false;
    // if hidden premit unloading
    return true;
}
//END DocumentObserver inherited methods


void PageView::showText( const QString &text, int ms )
{
    d->messageWindow->display(text, PageViewMessage::Info, ms );
}


//BEGIN widget events
void PageView::viewportPaintEvent( QPaintEvent * pe )
{
    // create the rect into contents from the clipped screen rect
    QRect viewportRect = viewport()->rect();
    QRect contentsRect = pe->rect().intersect( viewportRect );
    contentsRect.moveBy( contentsX(), contentsY() );
    if ( !contentsRect.isValid() )
        return;

    // create the screen painter. a pixel painted ar contentsX,contentsY
    // appears to the top-left corner of the scrollview.
    QPainter screenPainter( viewport(), true );
    screenPainter.translate( -contentsX(), -contentsY() );

    // selectionRect is the normalized mouse selection rect
    QRect selectionRect = d->mouseSelectionRect;
    if ( !selectionRect.isNull() )
        selectionRect = selectionRect.normalize();
    // selectionRectInternal without the border
    QRect selectionRectInternal = selectionRect;
    selectionRectInternal.addCoords( 1, 1, -1, -1 );
    // color for blending
    QColor selBlendColor = (selectionRect.width() > 8 || selectionRect.height() > 8) ?
                           d->selectionRectColor : Qt::red;

    // subdivide region into rects
    QMemArray<QRect> allRects = pe->region().rects();
    uint numRects = allRects.count();

    // preprocess rects area to see if it worths or not using subdivision
    uint summedArea = 0;
    for ( uint i = 0; i < numRects; i++ )
    {
        const QRect & r = allRects[i];
        summedArea += r.width() * r.height();
    }
    // very elementary check: SUMj(Region[j].area) is less than boundingRect.area
    bool useSubdivision = summedArea < (0.7 * contentsRect.width() * contentsRect.height());
    if ( !useSubdivision )
        numRects = 1;

    // iterate over the rects (only one loop if not using subdivision)
    for ( uint i = 0; i < numRects; i++ )
    {
        if ( useSubdivision )
        {
            // set 'contentsRect' to a part of the sub-divided region
            contentsRect = allRects[i].normalize().intersect( viewportRect );
            contentsRect.moveBy( contentsX(), contentsY() );
            if ( !contentsRect.isValid() )
                continue;
        }

        // note: this check will take care of all things requiring alpha blending (not only selection)
        bool wantCompositing = !selectionRect.isNull() && contentsRect.intersects( selectionRect );

        if ( wantCompositing && KpdfSettings::enableCompositing() )
        {
            // create pixmap and open a painter over it (contents{left,top} becomes pixmap {0,0})
            QPixmap doubleBuffer( contentsRect.size() );
            QPainter pixmapPainter( &doubleBuffer );
            pixmapPainter.translate( -contentsRect.left(), -contentsRect.top() );

            // 1) Layer 0: paint items and clear bg on unpainted rects
            paintItems( &pixmapPainter, contentsRect );
            // 2) Layer 1: pixmap manipulated areas
            // 3) Layer 2: paint (blend) transparent selection
            if ( !selectionRect.isNull() && selectionRect.intersects( contentsRect ) &&
                 !selectionRectInternal.contains( contentsRect ) )
            {
                QRect blendRect = selectionRectInternal.intersect( contentsRect );
                // skip rectangles covered by the selection's border
                if ( blendRect.isValid() )
                {
                    // grab current pixmap into a new one to colorize contents
                    QPixmap blendedPixmap( blendRect.width(), blendRect.height() );
                    copyBlt( &blendedPixmap, 0,0, &doubleBuffer,
                                blendRect.left() - contentsRect.left(), blendRect.top() - contentsRect.top(),
                                blendRect.width(), blendRect.height() );
                    // blend selBlendColor into the background pixmap
                    QImage blendedImage = blendedPixmap.convertToImage();
                    KImageEffect::blend( selBlendColor.dark(140), blendedImage, 0.2 );
                    // copy the blended pixmap back to its place
                    pixmapPainter.drawPixmap( blendRect.left(), blendRect.top(), blendedImage );
                }
                // draw border (red if the selection is too small)
                pixmapPainter.setPen( selBlendColor );
                pixmapPainter.drawRect( selectionRect );
            }
            // 4) Layer 3: overlays
            if ( KpdfSettings::debugDrawBoundaries() )
            {
                pixmapPainter.setPen( Qt::blue );
                pixmapPainter.drawRect( contentsRect );
            }

            // finish painting and draw contents
            pixmapPainter.end();
            screenPainter.drawPixmap( contentsRect.left(), contentsRect.top(), doubleBuffer );
        }
        else
        {
            // 1) Layer 0: paint items and clear bg on unpainted rects
            paintItems( &screenPainter, contentsRect );
            // 2) Layer 1: opaque manipulated ares (filled / contours)
            // 3) Layer 2: paint opaque selection
            if ( !selectionRect.isNull() && selectionRect.intersects( contentsRect ) &&
                 !selectionRectInternal.contains( contentsRect ) )
            {
                screenPainter.setPen( palette().active().highlight().dark(110) );
                screenPainter.drawRect( selectionRect );
            }
            // 4) Layer 3: overlays
            if ( KpdfSettings::debugDrawBoundaries() )
            {
                screenPainter.setPen( Qt::red );
                screenPainter.drawRect( contentsRect );
            }
        }
    }
}

void PageView::viewportResizeEvent( QResizeEvent * )
{
    // start a timer that will refresh the pixmap after 0.5s
    if ( !d->delayResizeTimer )
    {
        d->delayResizeTimer = new QTimer( this );
        connect( d->delayResizeTimer, SIGNAL( timeout() ), this, SLOT( slotRelayoutPages() ) );
    }
    d->delayResizeTimer->start( 333, true );
}

void PageView::keyPressEvent( QKeyEvent * e )
{
    e->accept();

    // if performing a selection or dyn zooming, disable keys handling
    if ( ( !d->mouseSelectionRect.isNull() && e->key() != Qt::Key_Escape ) || d->mouseMidStartY != -1 )
        return;

    // handle 'find as you type' (based on khtml/khtmlview.cpp)
    if( d->typeAheadActive )
    {
        // backspace: remove a char and search or terminates search
        if( e->key() == Key_BackSpace )
        {
            if( d->typeAheadString.length() > 1 )
            {
                d->typeAheadString = d->typeAheadString.left( d->typeAheadString.length() - 1 );
                bool found = d->document->searchText( PAGEVIEW_SEARCH_ID, d->typeAheadString, true, false,
                        KPDFDocument::NextMatch, true, qRgb( 128, 255, 128 ), true );
                QString status = found ? i18n("Text found: \"%1\".") : i18n("Text not found: \"%1\".");
                d->messageWindow->display( status.arg(d->typeAheadString.lower()),
                                           found ? PageViewMessage::Find : PageViewMessage::Warning, 4000 );
                d->findTimeoutTimer->start( 3000, true );
            }
            else
            {
                findAheadStop();
                d->document->resetSearch( PAGEVIEW_SEARCH_ID );
            }
        }
        // F3: go to next occurrency
        else if( e->key() == KStdAccel::findNext() )
        {
            // part doesn't get this key event because of the keyboard grab
            d->findTimeoutTimer->stop(); // restore normal operation during possible messagebox is displayed
            // it is needed to grab the keyboard becase people may have Space assigned to a 
            // accel and without grabbing the keyboard you can not vim-search for space
            // because it activates the accel
            releaseKeyboard();
            if ( d->document->continueSearch( PAGEVIEW_SEARCH_ID ) )
                d->messageWindow->display( i18n("Text found: \"%1\".").arg(d->typeAheadString.lower()),
                                           PageViewMessage::Find, 3000 );
            d->findTimeoutTimer->start( 3000, true );
            // it is needed to grab the keyboard becase people may have Space assigned to a 
            // accel and without grabbing the keyboard you can not vim-search for space
            // because it activates the accel
            grabKeyboard();
        }
        // esc and return: end search
        else if( e->key() == Key_Escape || e->key() == Key_Return )
        {
            findAheadStop();
        }
        // other key: add to text and search
        else if( !e->text().isEmpty() )
        {
            d->typeAheadString += e->text();
            doTypeAheadSearch();
        }
        return;
    }
    else if( e->key() == '/' && d->document->isOpened() && d->document->supportsSearching() )
    {
        // stop scrolling the page (if doing it)
        if ( d->autoScrollTimer )
        {
            d->scrollIncrement = 0;
            d->autoScrollTimer->stop();
        }
        // start type-adeas search
        d->typeAheadString = QString();
        d->messageWindow->display( i18n("Starting -- find text as you type"), PageViewMessage::Find, 3000 );
        d->typeAheadActive = true;
        if ( !d->findTimeoutTimer )
        {
            // create the timer on demand
            d->findTimeoutTimer = new QTimer( this );
            connect( d->findTimeoutTimer, SIGNAL( timeout() ), this, SLOT( findAheadStop() ) );
        }
        d->findTimeoutTimer->start( 3000, true );
        // it is needed to grab the keyboard becase people may have Space assigned to a 
        // accel and without grabbing the keyboard you can not vim-search for space
        // because it activates the accel
        grabKeyboard();
        return;
    }

    // if viewport is moving, disable keys handling
    if ( d->viewportMoveActive )
        return;

    // move/scroll page by using keys
    switch ( e->key() )
    {
        case Key_Up:
        case Key_PageUp:
	case Key_Backspace:
            // if in single page mode and at the top of the screen, go to previous page
            if ( KpdfSettings::viewContinuous() || verticalScrollBar()->value() > verticalScrollBar()->minValue() )
            {
                if ( e->key() == Key_Up )
                    verticalScrollBar()->subtractLine();
                else
                    verticalScrollBar()->subtractPage();
            }
            else if ( d->document->currentPage() > 0 )
            {
                // more optimized than document->setPrevPage and then move view to bottom
                DocumentViewport newViewport = d->document->viewport();
                newViewport.pageNumber -= 1;
                newViewport.rePos.enabled = true;
                newViewport.rePos.normalizedY = 1.0;
                d->document->setViewport( newViewport );
            }
            break;
        case Key_Down:
        case Key_PageDown:
	case Key_Space:
            // if in single page mode and at the bottom of the screen, go to next page
            if ( KpdfSettings::viewContinuous() || verticalScrollBar()->value() < verticalScrollBar()->maxValue() )
            {
                if ( e->key() == Key_Down )
                    verticalScrollBar()->addLine();
                else
                    verticalScrollBar()->addPage();
            }
            else if ( d->document->currentPage() < d->items.count() - 1 )
            {
                // more optmized than document->setNextPage and then move view to top
                DocumentViewport newViewport = d->document->viewport();
                newViewport.pageNumber += 1;
                newViewport.rePos.enabled = true;
                newViewport.rePos.normalizedY = 0.0;
                d->document->setViewport( newViewport );
            }
            break;
        case Key_Left:
            horizontalScrollBar()->subtractLine();
            break;
        case Key_Right:
            horizontalScrollBar()->addLine();
            break;
        case Qt::Key_Escape:
            selectionClear();
            d->mousePressPos = QPoint();
            if ( d->aPrevAction )
            {
                d->aPrevAction->activate();
                d->aPrevAction = 0;
            }
            break;
        case Key_Shift:
        case Key_Control:
            if ( d->autoScrollTimer )
            {
                if ( d->autoScrollTimer->isActive() )
                    d->autoScrollTimer->stop();
                else
                    slotAutoScoll();
                return;
            }
            // else fall trhough
        default:
            e->ignore();
            return;
    }
    // if a known key has been pressed, stop scrolling the page
    if ( d->autoScrollTimer )
    {
        d->scrollIncrement = 0;
        d->autoScrollTimer->stop();
    }
}

void PageView::imEndEvent( QIMEvent * e )
{
    if( d->typeAheadActive )
    {
        if( !e->text().isEmpty() )
        {
            d->typeAheadString += e->text();
            doTypeAheadSearch();
            e->accept();
        }
    }
}

void PageView::contentsMouseMoveEvent( QMouseEvent * e )
{
    // don't perform any mouse action when no document is shown
    if ( d->items.isEmpty() )
        return;

    // don't perform any mouse action when viewport is autoscrolling
    if ( d->viewportMoveActive )
        return;

    // if holding mouse mid button, perform zoom
    if ( (e->state() & MidButton) && d->mouseMidStartY >= 0 )
    {
        int deltaY = d->mouseMidStartY - e->globalPos().y();
        d->mouseMidStartY = e->globalPos().y();
        d->zoomFactor *= ( 1.0 + ( (double)deltaY / 500.0 ) );
        updateZoom( ZoomRefreshCurrent );
        // uncomment following line to force a complete redraw
        viewport()->repaint( false );
        return;
    }

    bool leftButton = e->state() & LeftButton,
         rightButton = e->state() & RightButton;
    switch ( d->mouseMode )
    {
        case MouseNormal:
            if ( leftButton )
            {
                // drag page
                if ( !d->mouseGrabPos.isNull() )
                {
                    // scroll page by position increment
                    QPoint delta = d->mouseGrabPos - e->globalPos();
                    scrollBy( delta.x(), delta.y() );
                    d->mouseGrabPos = e->globalPos();
                }
            }
            else if ( rightButton && !d->mousePressPos.isNull() )
            {
                // if mouse moves 5 px away from the press point, switch to 'selection'
                int deltaX = d->mousePressPos.x() - e->globalPos().x(),
                    deltaY = d->mousePressPos.y() - e->globalPos().y();
                if ( deltaX > 5 || deltaX < -5 || deltaY > 5 || deltaY < -5 )
                {
                    d->aPrevAction = d->aMouseNormal;
                    d->aMouseSelect->activate();
                    QColor selColor = palette().active().highlight().light( 120 );
                    selectionStart( e->x() + deltaX, e->y() + deltaY, selColor, false );
                    selectionEndPoint( e->x(), e->y() );
                    break;
                }
            }
            else
            {
                // only hovering the page, so update the cursor
                updateCursor( e->pos() );
            }
            break;

        case MouseZoom:
        case MouseSelect:
            // set second corner of selection
            if ( !d->mousePressPos.isNull() && ( leftButton || d->aPrevAction ) )
                selectionEndPoint( e->x(), e->y() );
            break;

        case MouseEdit:      // ? update graphics ?
            break;
    }
}

void PageView::contentsMousePressEvent( QMouseEvent * e )
{
    // don't perform any mouse action when no document is shown
    if ( d->items.isEmpty() )
        return;

    // if performing a selection or dyn zooming, disable mouse press
    if ( !d->mouseSelectionRect.isNull() || d->mouseMidStartY != -1 ||
         d->viewportMoveActive )
        return;

    // if the page is scrolling, stop it
    if ( d->autoScrollTimer )
    {
        d->scrollIncrement = 0;
        d->autoScrollTimer->stop();
    }

    // if pressing mid mouse button while not doing other things, begin 'comtinous zoom' mode
    if ( e->button() & MidButton )
    {
        d->mouseMidStartY = e->globalPos().y();
        setCursor( KCursor::sizeVerCursor() );
        return;
    }

    // update press / 'start drag' mouse position
    d->mousePressPos = e->globalPos();

    // handle mode dependant mouse press actions
    bool leftButton = e->button() & LeftButton,
         rightButton = e->button() & RightButton;
    switch ( d->mouseMode )
    {
        case MouseNormal:   // drag start / click / link following
            if ( leftButton )
            {
                d->mouseGrabPos = d->mouseOnRect ? QPoint() : d->mousePressPos;
                if ( !d->mouseOnRect )
                    setCursor( KCursor::sizeAllCursor() );
            }
            break;

        case MouseZoom:     // set first corner of the zoom rect
            if ( leftButton )
                selectionStart( e->x(), e->y(), palette().active().highlight(), false );
            else if ( rightButton )
                updateZoom( ZoomOut );
            break;

        case MouseSelect:   // set first corner of the selection rect
            if ( leftButton )
            {
                QColor selColor = palette().active().highlight().light( 120 );
                selectionStart( e->x(), e->y(), selColor, false );
            }
            break;

        case MouseEdit:     // ..to do..
            break;
    }
}

void PageView::contentsMouseReleaseEvent( QMouseEvent * e )
{
    // stop the drag scrolling
    d->dragScrollTimer.stop();

    // don't perform any mouse action when no document is shown
    if ( d->items.isEmpty() )
    {
        // ..except for right Clicks (emitted even it viewport is empty)
        if ( e->button() == RightButton )
            emit rightClick( 0, e->globalPos() );
        return;
    }

    // don't perform any mouse action when viewport is autoscrolling
    if ( d->viewportMoveActive )
        return;

    // handle mode indepent mid buttom zoom
    bool midButton = e->button() & MidButton;
    if ( midButton && d->mouseMidStartY > 0 )
    {
        d->mouseMidStartY = -1;
        // while drag-zooming we could have gone over a link
        updateCursor( e->pos() );
        return;
    }

    bool leftButton = e->button() & LeftButton,
         rightButton = e->button() & RightButton;
    switch ( d->mouseMode )
    {
        case MouseNormal:{
            // return the cursor to its normal state after dragging
            if ( cursor().shape() == Qt::SizeAllCursor )
                updateCursor( e->pos() );

            PageViewItem * pageItem = pickItemOnPoint( e->x(), e->y() );

            // if the mouse has not moved since the press, that's a -click-
            if ( leftButton && pageItem && d->mousePressPos == e->globalPos())
            {
                double nX = (double)(e->x() - pageItem->geometry().left()) / (double)pageItem->width(),
                       nY = (double)(e->y() - pageItem->geometry().top()) / (double)pageItem->height();
                const ObjectRect * linkRect, * imageRect;
                linkRect = pageItem->page()->hasObject( ObjectRect::Link, nX, nY );
                if ( linkRect )
                {
                    // handle click over a link
                    const KPDFLink * link = static_cast< const KPDFLink * >( linkRect->pointer() );
                    d->document->processLink( link );
                }
                else
                {
                    // a link can move us to another page or even to another document, there's no point in trying to process the click on the image once we have processes the click on the link
                    imageRect = pageItem->page()->hasObject( ObjectRect::Image, nX, nY );
                    if ( imageRect )
                    {
                        // handle click over a image
                    }
                    // Enrico and me have decided this is not worth the trouble it generates
                    // else
                    // {
                        // if not on a rect, the click selects the page
                        // d->document->setViewportPage( pageItem->pageNumber(), PAGEVIEW_ID );
                    // }
                }
            }
            else if ( rightButton )
            {
                // right click (if not within 5 px of the press point, the mode
                // had been already changed to 'Selection' instead of 'Normal')
                emit rightClick( pageItem ? pageItem->page() : 0, e->globalPos() );
            }
            }break;

        case MouseZoom:
            // if a selection rect has been defined, zoom into it
            if ( leftButton && !d->mouseSelectionRect.isNull() )
            {
                QRect selRect = d->mouseSelectionRect.normalize();
                if ( selRect.width() <= 8 && selRect.height() <= 8 )
                {
                    selectionClear();
                    break;
                }

                // find out new zoom ratio and normalized view center (relative to the contentsRect)
                double zoom = QMIN( (double)visibleWidth() / (double)selRect.width(), (double)visibleHeight() / (double)selRect.height() );
                double nX = (double)(selRect.left() + selRect.right()) / (2.0 * (double)contentsWidth());
                double nY = (double)(selRect.top() + selRect.bottom()) / (2.0 * (double)contentsHeight());

                // zoom up to 400%
                if ( d->zoomFactor <= 4.0 || zoom <= 1.0 )
                {
                    d->zoomFactor *= zoom;
                    viewport()->setUpdatesEnabled( false );
                    updateZoom( ZoomRefreshCurrent );
                    viewport()->setUpdatesEnabled( true );
                }

                // recenter view and update the viewport
                center( (int)(nX * contentsWidth()), (int)(nY * contentsHeight()) );
                updateContents();

                // hide message box and delete overlay window
                selectionClear();
            }
            break;

        case MouseSelect:{

            if (d->mouseSelectionRect.isNull() && rightButton) 
	    {
	    	 PageViewItem * pageItem = pickItemOnPoint( e->x(), e->y() );
	    	 emit rightClick( pageItem ? pageItem->page() : 0, e->globalPos() );
	    }
	    
            // if a selection is defined, display a popup
            if ( (!leftButton && !d->aPrevAction) || (leftButton && d->aPrevAction) ||
                 d->mouseSelectionRect.isNull() )
                break;

            QRect selectionRect = d->mouseSelectionRect.normalize();
            if ( selectionRect.width() <= 8 && selectionRect.height() <= 8 )
            {
                selectionClear();
                if ( d->aPrevAction )
                {
                    d->aPrevAction->activate();
                    d->aPrevAction = 0;
                }
                break;
            }

            // grab text in selection by extracting it from all intersected pages
            QString selectedText;
            QValueVector< PageViewItem * >::iterator iIt = d->items.begin(), iEnd = d->items.end();
            for ( ; iIt != iEnd; ++iIt )
            {
                PageViewItem * item = *iIt;
                const QRect & itemRect = item->geometry();
                if ( selectionRect.intersects( itemRect ) )
                {
                    // request the textpage if there isn't one
                    const KPDFPage * kpdfPage = item->page();
                    if ( !kpdfPage->hasSearchPage() )
                        d->document->requestTextPage( kpdfPage->number() );
                    // grab text in the rect that intersects itemRect
                    QRect relativeRect = selectionRect.intersect( itemRect );
                    relativeRect.moveBy( -itemRect.left(), -itemRect.top() );
                    NormalizedRect normRect( relativeRect, item->width(), item->height() );
                    selectedText += kpdfPage->getText( normRect );
                }
            }

            // popup that ask to copy:text and copy/save:image
            KPopupMenu menu( this );
            if ( !selectedText.isEmpty() )
            {
                menu.insertTitle( i18n( "Text (1 character)", "Text (%n characters)", selectedText.length() ) );
                menu.insertItem( SmallIcon("editcopy"), i18n( "Copy to Clipboard" ), 1 );
                if ( !d->document->isAllowed( KPDFDocument::AllowCopy ) )
                    menu.setItemEnabled( 1, false );
                if ( KpdfSettings::useKTTSD() )
                    menu.insertItem( SmallIcon("kttsd"), i18n( "Speak Text" ), 2 );
            }
            menu.insertTitle( i18n( "Image (%1 by %2 pixels)" ).arg( selectionRect.width() ).arg( selectionRect.height() ) );
            menu.insertItem( SmallIcon("image"), i18n( "Copy to Clipboard" ), 3 );
            menu.insertItem( SmallIcon("filesave"), i18n( "Save to File..." ), 4 );
            int choice = menu.exec( e->globalPos() );
            // IMAGE operation choosen
            if ( choice > 2 )
            {
                // renders page into a pixmap
                QPixmap copyPix( selectionRect.width(), selectionRect.height() );
                QPainter copyPainter( &copyPix );
                copyPainter.translate( -selectionRect.left(), -selectionRect.top() );
                paintItems( &copyPainter, selectionRect );

                if ( choice == 3 )
                {
                    // [2] copy pixmap to clipboard
                    QClipboard *cb = QApplication::clipboard();
                    cb->setPixmap( copyPix, QClipboard::Clipboard );
                    if ( cb->supportsSelection() )
                        cb->setPixmap( copyPix, QClipboard::Selection );
                    d->messageWindow->display( i18n( "Image [%1x%2] copied to clipboard." ).arg( copyPix.width() ).arg( copyPix.height() ) );
                }
                else if ( choice == 4 )
                {
                    // [3] save pixmap to file
                    QString fileName = KFileDialog::getSaveFileName( QString::null, "image/png image/jpeg", this );
                    if ( fileName.isNull() )
                        d->messageWindow->display( i18n( "File not saved." ), PageViewMessage::Warning );
                    else
                    {
                        QString type( KImageIO::type( fileName ) );
                        if ( type.isNull() )
                            type = "PNG";
                        copyPix.save( fileName, type.latin1() );
                        d->messageWindow->display( i18n( "Image [%1x%2] saved to %3 file." ).arg( copyPix.width() ).arg( copyPix.height() ).arg( type ) );
                    }
                }
            }
            // TEXT operation choosen
            else
            {
                if ( choice == 1 )
                {
                    // [1] copy text to clipboard
                    QClipboard *cb = QApplication::clipboard();
                    cb->setText( selectedText, QClipboard::Clipboard );
                    if ( cb->supportsSelection() )
                        cb->setText( selectedText, QClipboard::Selection );
                }
                else if ( choice == 2 )
                {
                    // [2] speech selection using KTTSD
                    DCOPClient * client = DCOPClient::mainClient();
                    // Albert says is this ever necessary?
                    // we already attached on Part constructor
                    // if ( !client->isAttached() )
                    //    client->attach();
                    // If KTTSD not running, start it.
                    if (!client->isApplicationRegistered("kttsd"))
                    {
                        QString error;
                        if (KApplication::startServiceByDesktopName("kttsd", QStringList(), &error))
                        {
                            d->messageWindow->display( i18n("Starting KTTSD Failed: %1").arg(error) );
                            KpdfSettings::setUseKTTSD(false);
                            KpdfSettings::writeConfig();
                        }
                    }
                    if ( KpdfSettings::useKTTSD() )
                    {
                        // serialize the text to speech (selectedText) and the
                        // preferred reader ("" is the default voice) ...
                        QByteArray data;
                        QDataStream arg( data, IO_WriteOnly );
                        arg << selectedText;
                        arg << QString();
                        QCString replyType;
                        QByteArray replyData;
                        // ..and send it to KTTSD
                        if (client->call( "kttsd", "KSpeech", "setText(QString,QString)", data, replyType, replyData, true ))
                        {
                            QByteArray  data2;
                            QDataStream arg2(data2, IO_WriteOnly);
                            arg2 << 0;
                            client->send("kttsd", "KSpeech", "startText(uint)", data2 );
                        }
                    }
                }
            }

            // clear widget selection and invalidate rect
            selectionClear();

            // restore previous action if came from it using right button
            if ( d->aPrevAction )
            {
                d->aPrevAction->activate();
                d->aPrevAction = 0;
            }
            }break;

        case MouseEdit:      // ? apply [tool] ?
            break;
    }

    // reset mouse press / 'drag start' position
    d->mousePressPos = QPoint();
}

void PageView::wheelEvent( QWheelEvent *e )
{
    // don't perform any mouse action when viewport is autoscrolling
    if ( d->viewportMoveActive )
        return;

    if ( !d->document->isOpened() )
    {
        QScrollView::wheelEvent( e );
        return;
    }

    int delta = e->delta(),
        vScroll = verticalScrollBar()->value();
    e->accept();
    if ( (e->state() & ControlButton) == ControlButton ) {
        if ( e->delta() < 0 )
            slotZoomOut();
        else
            slotZoomIn();
    }
    else if ( delta <= -120 && !KpdfSettings::viewContinuous() && vScroll == verticalScrollBar()->maxValue() )
    {
        // go to next page
        if ( d->document->currentPage() < d->items.count() - 1 )
        {
            // more optmized than document->setNextPage and then move view to top
            DocumentViewport newViewport = d->document->viewport();
            newViewport.pageNumber += 1;
            newViewport.rePos.enabled = true;
            newViewport.rePos.normalizedY = 0.0;
            d->document->setViewport( newViewport );
        }
    }
    else if ( delta >= 120 && !KpdfSettings::viewContinuous() && vScroll == verticalScrollBar()->minValue() )
    {
        // go to prev page
        if ( d->document->currentPage() > 0 )
        {
            // more optmized than document->setPrevPage and then move view to bottom
            DocumentViewport newViewport = d->document->viewport();
            newViewport.pageNumber -= 1;
            newViewport.rePos.enabled = true;
            newViewport.rePos.normalizedY = 1.0;
            d->document->setViewport( newViewport );
        }
    }
    else
        QScrollView::wheelEvent( e );

    QPoint cp = viewportToContents(e->pos());
    updateCursor(cp);
}

void PageView::dragEnterEvent( QDragEnterEvent * ev )
{
    ev->accept();
}

void PageView::dropEvent( QDropEvent * ev )
{
    KURL::List lst;
    if (  KURLDrag::decode(  ev, lst ) )
        emit urlDropped( lst.first() );
}
//END widget events

void PageView::paintItems( QPainter * p, const QRect & contentsRect )
{
    // when checking if an Item is contained in contentsRect, instead of
    // growing PageViewItems rects (for keeping outline into account), we
    // grow the contentsRect
    QRect checkRect = contentsRect;
    checkRect.addCoords( -3, -3, 1, 1 );

    // create a region from wich we'll subtract painted rects
    QRegion remainingArea( contentsRect );

    //QValueVector< PageViewItem * >::iterator iIt = d->visibleItems.begin(), iEnd = d->visibleItems.end();
    QValueVector< PageViewItem * >::iterator iIt = d->items.begin(), iEnd = d->items.end();
    for ( ; iIt != iEnd; ++iIt )
    {
        // check if a piece of the page intersects the contents rect
        if ( !(*iIt)->geometry().intersects( checkRect ) )
            continue;

        PageViewItem * item = *iIt;
        QRect pixmapGeometry = item->geometry();

        // translate the painter so we draw top-left pixmap corner in 0,0
        p->save();
        p->translate( pixmapGeometry.left(), pixmapGeometry.top() );

        // item pixmap and outline geometry
        QRect outlineGeometry = pixmapGeometry;
        outlineGeometry.addCoords( -1, -1, 3, 3 );

        // draw the page outline (little black border and 2px shadow)
        if ( !pixmapGeometry.contains( contentsRect ) )
        {
            int pixmapWidth = pixmapGeometry.width(),
                pixmapHeight = pixmapGeometry.height();
            // draw simple outline
            p->setPen( Qt::black );
            p->drawRect( -1, -1, pixmapWidth + 2, pixmapHeight + 2 );
            // draw bottom/right gradient
            int levels = 2;
            int r = Qt::gray.red() / (levels + 2),
                g = Qt::gray.green() / (levels + 2),
                b = Qt::gray.blue() / (levels + 2);
            for ( int i = 0; i < levels; i++ )
            {
                p->setPen( QColor( r * (i+2), g * (i+2), b * (i+2) ) );
                p->drawLine( i, i + pixmapHeight + 1, i + pixmapWidth + 1, i + pixmapHeight + 1 );
                p->drawLine( i + pixmapWidth + 1, i, i + pixmapWidth + 1, i + pixmapHeight );
                p->setPen( Qt::gray );
                p->drawLine( -1, i + pixmapHeight + 1, i - 1, i + pixmapHeight + 1 );
                p->drawLine( i + pixmapWidth + 1, -1, i + pixmapWidth + 1, i - 1 );
            }
        }

        // draw the pixmap (note: this modifies the painter)
        if ( contentsRect.intersects( pixmapGeometry ) )
        {
            QRect pixmapRect = contentsRect.intersect( pixmapGeometry );
            pixmapRect.moveBy( -pixmapGeometry.left(), -pixmapGeometry.top() );
            int flags = PagePainter::Accessibility | PagePainter::EnhanceLinks |
                        PagePainter::EnhanceImages | PagePainter::Highlights;
            PagePainter::paintPageOnPainter( item->page(), PAGEVIEW_ID, flags, p, pixmapRect,
                                             pixmapGeometry.width(), pixmapGeometry.height() );
        }

        // remove painted area from 'remainingArea' and restore painter
        remainingArea -= outlineGeometry.intersect( contentsRect );
        p->restore();
    }

    // paint with background color the unpainted area
    QMemArray<QRect> backRects = remainingArea.rects();
    uint backRectsNumber = backRects.count();
    for ( uint jr = 0; jr < backRectsNumber; jr++ )
        p->fillRect( backRects[ jr ], Qt::gray );
}

void PageView::updateItemSize( PageViewItem * item, int colWidth, int rowHeight )
{
    const KPDFPage * kpdfPage = item->page();
    double width = kpdfPage->width(),
           height = kpdfPage->height(),
           zoom = d->zoomFactor;

    if ( d->zoomMode == ZoomFixed )
    {
        width *= zoom;
        height *= zoom;
        item->setWHZ( (int)width, (int)height, d->zoomFactor );
    }
    else if ( d->zoomMode == ZoomFitWidth )
    {
        height = kpdfPage->ratio() * colWidth;
        item->setWHZ( colWidth, (int)height, (double)colWidth / width );
        d->zoomFactor = (double)colWidth / width;
    }
    else if ( d->zoomMode == ZoomFitPage )
    {
        double scaleW = (double)colWidth / (double)width;
        double scaleH = (double)rowHeight / (double)height;
        zoom = QMIN( scaleW, scaleH );
        item->setWHZ( (int)(zoom * width), (int)(zoom * height), zoom );
        d->zoomFactor = zoom;
    }
#ifndef NDEBUG
    else
        kdDebug() << "calling updateItemSize with unrecognized d->zoomMode!" << endl;
#endif
}

PageViewItem * PageView::pickItemOnPoint( int x, int y )
{
    PageViewItem * item = 0;
    QValueList< PageViewItem * >::iterator iIt = d->visibleItems.begin(), iEnd = d->visibleItems.end();
    for ( ; iIt != iEnd; ++iIt )
    {
        PageViewItem * i = *iIt;
        const QRect & r = i->geometry();
        if ( x < r.right() && x > r.left() && y < r.bottom() )
        {
            if ( y > r.top() )
                item = i;
            break;
        }
    }
    return item;
}

void PageView::selectionStart( int x, int y, const QColor & color, bool /*aboveAll*/ )
{
    d->mouseSelectionRect.setRect( x, y, 1, 1 );
    d->selectionRectColor = color;
    // ensures page doesn't scroll
    if ( d->autoScrollTimer )
    {
        d->scrollIncrement = 0;
        d->autoScrollTimer->stop();
    }
}

void PageView::selectionEndPoint( int x, int y )
{
    if (x < contentsX()) d->dragScrollVector.setX(x - contentsX());
    else if (contentsX() + viewport()->width() < x) d->dragScrollVector.setX(x - contentsX() - viewport()->width());
    else d->dragScrollVector.setX(0);

    if (y < contentsY()) d->dragScrollVector.setY(y - contentsY());
    else if (contentsY() + viewport()->height() < y) d->dragScrollVector.setY(y - contentsY() - viewport()->height());
    else d->dragScrollVector.setY(0);

    if (d->dragScrollVector != QPoint(0, 0))
    {
       if (!d->dragScrollTimer.isActive()) d->dragScrollTimer.start(100);
    }
    else d->dragScrollTimer.stop();

    // clip selection to the viewport
    QRect viewportRect( contentsX(), contentsY(), visibleWidth(), visibleHeight() );
    x = QMAX( QMIN( x, viewportRect.right() ), viewportRect.left() );
    y = QMAX( QMIN( y, viewportRect.bottom() ), viewportRect.top() );
    // if selection changed update rect
    if ( d->mouseSelectionRect.right() != x || d->mouseSelectionRect.bottom() != y )
    {
        // send incremental paint events
        QRect oldRect = d->mouseSelectionRect.normalize();
        d->mouseSelectionRect.setRight( x );
        d->mouseSelectionRect.setBottom( y );
        QRect newRect = d->mouseSelectionRect.normalize();
        // generate diff region: [ OLD.unite(NEW) - OLD.intersect(NEW) ]
        QRegion compoundRegion = QRegion( oldRect ).unite( newRect );
        if ( oldRect.intersects( newRect ) )
        {
            QRect intersection = oldRect.intersect( newRect );
            intersection.addCoords( 1, 1, -1, -1 );
            if ( intersection.width() > 20 && intersection.height() > 20 )
                compoundRegion -= intersection;
        }
        // tassellate region with rects and enqueue paint events
        QMemArray<QRect> rects = compoundRegion.rects();
        for ( uint i = 0; i < rects.count(); i++ )
            updateContents( rects[i] );
    }
}

void PageView::selectionClear()
{
    updateContents( d->mouseSelectionRect.normalize() );
    d->mouseSelectionRect.setCoords( 0, 0, -1, -1 );
}

void PageView::updateZoom( ZoomMode newZoomMode )
{
    if ( newZoomMode == ZoomFixed )
    {
        if ( d->aZoom->currentItem() == 0 )
            newZoomMode = ZoomFitWidth;
        else if ( d->aZoom->currentItem() == 1 )
            newZoomMode = ZoomFitPage;
    }

    float newFactor = d->zoomFactor;
    KAction * checkedZoomAction = 0;
    switch ( newZoomMode )
    {
        case ZoomFixed:{ //ZoomFixed case
            QString z = d->aZoom->currentText();
            newFactor = KGlobal::locale()->readNumber( z.remove( z.find( '%' ), 1 ) ) / 100.0;
            }break;
        case ZoomIn:
            newFactor += (newFactor > 0.99) ? ( newFactor > 1.99 ? 0.5 : 0.2 ) : 0.1;
            newZoomMode = ZoomFixed;
            break;
        case ZoomOut:
            newFactor -= (newFactor > 0.99) ? ( newFactor > 1.99 ? 0.5 : 0.2 ) : 0.1;
            newZoomMode = ZoomFixed;
            break;
        case ZoomFitWidth:
            checkedZoomAction = d->aZoomFitWidth;
            break;
        case ZoomFitPage:
            checkedZoomAction = d->aZoomFitPage;
            break;
        case ZoomFitText:
            checkedZoomAction = d->aZoomFitText;
            break;
        case ZoomRefreshCurrent:
            newZoomMode = ZoomFixed;
            d->zoomFactor = -1;
            break;
    }
    if ( newFactor > 4.0 )
        newFactor = 4.0;
    if ( newFactor < 0.1 )
        newFactor = 0.1;

    if ( newZoomMode != d->zoomMode || (newZoomMode == ZoomFixed && newFactor != d->zoomFactor ) )
    {
        // rebuild layout and update the whole viewport
        d->zoomMode = newZoomMode;
        d->zoomFactor = newFactor;
        // be sure to block updates to document's viewport
        bool prevState = d->blockViewport;
        d->blockViewport = true;
        slotRelayoutPages();
        d->blockViewport = prevState;
        // request pixmaps
        slotRequestVisiblePixmaps();
        // update zoom text
        updateZoomText();
        // update actions checked state
        d->aZoomFitWidth->setChecked( checkedZoomAction == d->aZoomFitWidth );
        d->aZoomFitPage->setChecked( checkedZoomAction == d->aZoomFitPage );
        d->aZoomFitText->setChecked( checkedZoomAction == d->aZoomFitText );
        
        // save selected zoom factor
        KpdfSettings::setZoomMode(newZoomMode);
        KpdfSettings::setZoomFactor(newFactor);
        KpdfSettings::writeConfig();
    }
}

void PageView::updateZoomText()
{
    // use current page zoom as zoomFactor if in ZoomFit/* mode
    if ( d->zoomMode != ZoomFixed && d->items.count() > 0 )
        d->zoomFactor = d->items[ QMAX( 0, (int)d->document->currentPage() ) ]->zoomFactor();
    float newFactor = d->zoomFactor;
    d->aZoom->clear();

    // add items that describe fit actions
    QStringList translated;
    translated << i18n("Fit Width") << i18n("Fit Page"); // << i18n("Fit Text");

    // add percent items
    QString double_oh( "00" );
    const float zoomValue[10] = { 0.125, 0.25, 0.333, 0.5, 0.667, 0.75, 1, 1.25, 1.50, 2 };
    int idx = 0,
        selIdx = 2; // use 3 if "fit text" present
    bool inserted = false; //use: "d->zoomMode != ZoomFixed" to hide Fit/* zoom ratio
    while ( idx < 10 || !inserted )
    {
        float value = idx < 10 ? zoomValue[ idx ] : newFactor;
        if ( !inserted && newFactor < (value - 0.0001) )
            value = newFactor;
        else
            idx ++;
        if ( value > (newFactor - 0.0001) && value < (newFactor + 0.0001) )
            inserted = true;
        if ( !inserted )
            selIdx++;
        QString localValue( KGlobal::locale()->formatNumber( value * 100.0, 2 ) );
        localValue.remove( KGlobal::locale()->decimalSymbol() + double_oh );
        translated << QString( "%1%" ).arg( localValue );
    }
    d->aZoom->setItems( translated );

    // select current item in list
    if ( d->zoomMode == ZoomFitWidth )
        selIdx = 0;
    else if ( d->zoomMode == ZoomFitPage )
        selIdx = 1;
    else if ( d->zoomMode == ZoomFitText )
        selIdx = 2;
    d->aZoom->setCurrentItem( selIdx );
}

void PageView::updateCursor( const QPoint &p )
{
    // detect the underlaying page (if present)
    PageViewItem * pageItem = pickItemOnPoint( p.x(), p.y() );
    if ( pageItem && d->mouseMode == MouseNormal )
    {
        double nX = (double)(p.x() - pageItem->geometry().left()) / (double)pageItem->width(),
               nY = (double)(p.y() - pageItem->geometry().top()) / (double)pageItem->height();

        // if over a ObjectRect (of type Link) change cursor to hand
        d->mouseOnRect = pageItem->page()->hasObject( ObjectRect::Link, nX, nY );
        if ( d->mouseOnRect )
            setCursor( KCursor::handCursor() );
        else
            setCursor( KCursor::arrowCursor() );
    }
    else
    {
        // if there's no page over the cursor and we were showing the pointingHandCursor
        // go back to the normal one
        d->mouseOnRect = false;
        setCursor( KCursor::arrowCursor() );
    }
}

void PageView::doTypeAheadSearch()
{
    bool found = d->document->searchText( PAGEVIEW_SEARCH_ID, d->typeAheadString, false, false,
                                          KPDFDocument::NextMatch, true, qRgb( 128, 255, 128 ), true );
    QString status = found ? i18n("Text found: \"%1\".") : i18n("Text not found: \"%1\".");
    d->messageWindow->display( status.arg(d->typeAheadString.lower()),
                               found ? PageViewMessage::Find : PageViewMessage::Warning, 4000 );
    d->findTimeoutTimer->start( 3000, true );
}

//BEGIN private SLOTS
void PageView::slotRelayoutPages()
// called by: notifySetup, viewportResizeEvent, slotTwoPagesToggled, slotContinuousToggled, updateZoom
{
    // set an empty container if we have no pages
    int pageCount = d->items.count();
    if ( pageCount < 1 )
    {
        resizeContents( 0, 0 );
        return;
    }

    // if viewport was auto-moving, stop it
    if ( d->viewportMoveActive )
    {
        d->viewportMoveActive = false;
        d->viewportMoveTimer->stop();
        verticalScrollBar()->setEnabled( true );
        horizontalScrollBar()->setEnabled( true );
    }

    // common iterator used in this method and viewport parameters
    QValueVector< PageViewItem * >::iterator iIt, iEnd = d->items.end();
    int viewportWidth = visibleWidth(),
        viewportHeight = visibleHeight(),
        fullWidth = 0,
        fullHeight = 0;
    QRect viewportRect( contentsX(), contentsY(), viewportWidth, viewportHeight );

    // set all items geometry and resize contents. handle 'continuous' and 'single' modes separately
    if ( KpdfSettings::viewContinuous() )
    {
        // Here we find out column's width and row's height to compute a table
        // so we can place widgets 'centered in virtual cells'.
        int nCols = KpdfSettings::viewColumns(),
            nRows = (int)ceil( (float)pageCount / (float)nCols ),
            * colWidth = new int[ nCols ],
            * rowHeight = new int[ nRows ],
            cIdx = 0,
            rIdx = 0;
        for ( int i = 0; i < nCols; i++ )
            colWidth[ i ] = viewportWidth / nCols;
        for ( int i = 0; i < nRows; i++ )
            rowHeight[ i ] = 0;

        // 1) find the maximum columns width and rows height for a grid in
        // which each page must well-fit inside a cell
        for ( iIt = d->items.begin(); iIt != iEnd; ++iIt )
        {
            PageViewItem * item = *iIt;
            // update internal page size (leaving a little margin in case of Fit* modes)
            updateItemSize( item, colWidth[ cIdx ] - 6, viewportHeight - 8 );
            // find row's maximum height and column's max width
            if ( item->width() + 6 > colWidth[ cIdx ] )
                colWidth[ cIdx ] = item->width() + 6;
            if ( item->height() > rowHeight[ rIdx ] )
                rowHeight[ rIdx ] = item->height();
            // update col/row indices
            if ( ++cIdx == nCols )
            {
                cIdx = 0;
                rIdx++;
            }
        }

        // 2) arrange widgets inside cells
        int insertX = 0,
            insertY = 4; // 2 + 4*d->zoomFactor ?
        cIdx = 0;
        rIdx = 0;
        for ( iIt = d->items.begin(); iIt != iEnd; ++iIt )
        {
            PageViewItem * item = *iIt;
            int cWidth = colWidth[ cIdx ],
                rHeight = rowHeight[ rIdx ];
            // center widget inside 'cells'
            item->moveTo( insertX + (cWidth - item->width()) / 2,
                          insertY + (rHeight - item->height()) / 2 );
            // advance col/row index
            insertX += cWidth;
            if ( ++cIdx == nCols )
            {
                cIdx = 0;
                rIdx++;
                insertX = 0;
                insertY += rHeight + 15; // 5 + 15*d->zoomFactor ?
            }
        }

        fullHeight = cIdx ? (insertY + rowHeight[ rIdx ] + 10) : insertY;
        for ( int i = 0; i < nCols; i++ )
            fullWidth += colWidth[ i ];

        delete [] colWidth;
        delete [] rowHeight;
    }
    else // viewContinuous is FALSE
    {
        PageViewItem * currentItem = d->items[ QMAX( 0, (int)d->document->currentPage() ) ];

        // setup varialbles for a 1(row) x N(columns) grid
        int nCols = KpdfSettings::viewColumns(),
            * colWidth = new int[ nCols ],
            cIdx = 0;
        fullHeight = viewportHeight;
        for ( int i = 0; i < nCols; i++ )
            colWidth[ i ] = viewportWidth / nCols;

        // 1) find out maximum area extension for the pages
        for ( iIt = d->items.begin(); iIt != iEnd; ++iIt )
        {
            PageViewItem * item = *iIt;
            if ( item == currentItem || (cIdx > 0 && cIdx < nCols) )
            {
                // update internal page size (leaving a little margin in case of Fit* modes)
                updateItemSize( item, colWidth[ cIdx ] - 6, viewportHeight - 8 );
                // find row's maximum height and column's max width
                if ( item->width() + 6 > colWidth[ cIdx ] )
                    colWidth[ cIdx ] = item->width() + 6;
                if ( item->height() + 8 > fullHeight )
                    fullHeight = item->height() + 8;
                cIdx++;
            }
        }

        // 2) hide all widgets except the displayable ones and dispose those
        int insertX = 0;
        cIdx = 0;
        for ( iIt = d->items.begin(); iIt != iEnd; ++iIt )
        {
            PageViewItem * item = *iIt;
            if ( item == currentItem || (cIdx > 0 && cIdx < nCols) )
            {
                // center widget inside 'cells'
                item->moveTo( insertX + (colWidth[ cIdx ] - item->width()) / 2,
                              (fullHeight - item->height()) / 2 );
                // advance col index
                insertX += colWidth[ cIdx ];
                cIdx++;
            } else
                item->setGeometry( 0, 0, -1, -1 );
        }

        for ( int i = 0; i < nCols; i++ )
            fullWidth += colWidth[ i ];

        delete [] colWidth;
    }

    // 3) reset dirty state
    d->dirtyLayout = false;

    // 4) update scrollview's contents size and recenter view
    bool wasUpdatesEnabled = viewport()->isUpdatesEnabled();
    if ( fullWidth != contentsWidth() || fullHeight != contentsHeight() )
    {
        // disable updates and resize the viewportContents
        if ( wasUpdatesEnabled )
            viewport()->setUpdatesEnabled( false );
        resizeContents( fullWidth, fullHeight );
        // restore previous viewport if defined and updates enabled
        if ( wasUpdatesEnabled )
        {
            const DocumentViewport & vp = d->document->viewport();
            if ( vp.pageNumber >= 0 )
            {
                int prevX = contentsX(),
                    prevY = contentsY();
                const QRect & geometry = d->items[ vp.pageNumber ]->geometry();
                double nX = vp.rePos.enabled ? vp.rePos.normalizedX : 0.5,
                       nY = vp.rePos.enabled ? vp.rePos.normalizedY : 0.0;
                center( geometry.left() + ROUND( nX * (double)geometry.width() ),
                        geometry.top() + ROUND( nY * (double)geometry.height() ) );
                // center() usually moves the viewport, that requests pixmaps too.
                // if that doesn't happen we have to request them by hand
                if ( prevX == contentsX() && prevY == contentsY() )
                    slotRequestVisiblePixmaps();
            }
            // or else go to center page
            else
                center( fullWidth / 2, 0 );
            viewport()->setUpdatesEnabled( true );
        }
    }

    // 5) update the whole viewport if updated enabled
    if ( wasUpdatesEnabled )
        updateContents();
}

void PageView::slotRequestVisiblePixmaps( int newLeft, int newTop )
{
    // if requests are blocked (because raised by an unwanted event), exit
    if ( d->blockPixmapsRequest || d->viewportMoveActive )
        return;

    // precalc view limits for intersecting with page coords inside the lOOp
    bool isEvent = newLeft != -1 && newTop != -1 && !d->blockViewport;
    QRect viewportRect( isEvent ? newLeft : contentsX(),
                        isEvent ? newTop : contentsY(),
                        visibleWidth(), visibleHeight() );

    // some variables used to determine the viewport
    int nearPageNumber = -1;
    double viewportCenterX = (viewportRect.left() + viewportRect.right()) / 2.0,
           viewportCenterY = (viewportRect.top() + viewportRect.bottom()) / 2.0,
           focusedX = 0.5,
           focusedY = 0.0,
           minDistance = -1.0;

    // iterate over all items
    d->visibleItems.clear();
    QValueList< PixmapRequest * > requestedPixmaps;
    QValueVector< PageViewItem * >::iterator iIt = d->items.begin(), iEnd = d->items.end();
    for ( ; iIt != iEnd; ++iIt )
    {
        PageViewItem * i = *iIt;

        // if the item doesn't intersect the viewport, skip it
        if ( !viewportRect.intersects( i->geometry() ) )
            continue;

        // add the item to the 'visible list'
        d->visibleItems.push_back( i );

        // if the item has not the right pixmap, add a request for it
        if ( !i->page()->hasPixmap( PAGEVIEW_ID, i->width(), i->height() ) )
        {
            PixmapRequest * p = new PixmapRequest(
                    PAGEVIEW_ID, i->pageNumber(), i->width(), i->height(), PAGEVIEW_PRIO, true );
            requestedPixmaps.push_back( p );
        }

        // look for the item closest to viewport center and the relative
        // position between the item and the viewport center
        if ( isEvent )
        {
            const QRect & geometry = i->geometry();
            // compute distance between item center and viewport center
            double distance = hypot( (geometry.left() + geometry.right()) / 2 - viewportCenterX,
                                     (geometry.top() + geometry.bottom()) / 2 - viewportCenterY );
            if ( distance >= minDistance && nearPageNumber != -1 )
                continue;
            nearPageNumber = i->pageNumber();
            minDistance = distance;
            if ( geometry.height() > 0 && geometry.width() > 0 )
            {
                focusedX = ( viewportCenterX - (double)geometry.left() ) / (double)geometry.width();
                focusedY = ( viewportCenterY - (double)geometry.top() ) / (double)geometry.height();
            }
        }
    }

    // if preloading is enabled, add the pages before and after in preloading
    if ( !d->visibleItems.isEmpty() &&
         KpdfSettings::memoryLevel() != KpdfSettings::EnumMemoryLevel::Low &&
         KpdfSettings::enableThreading() )
    {
        // as the requests are done in the order as they appear in the list,
        // request first the next page and then the previous

        // add the page after the 'visible series' in preload
        int tailRequest = d->visibleItems.last()->pageNumber() + 1;
        if ( tailRequest < (int)d->items.count() )
        {
            PageViewItem * i = d->items[ tailRequest ];
            // request the pixmap if not already present
            if ( !i->page()->hasPixmap( PAGEVIEW_ID, i->width(), i->height() ) && i->width() > 0 )
                requestedPixmaps.push_back( new PixmapRequest(
                        PAGEVIEW_ID, i->pageNumber(), i->width(), i->height(), PAGEVIEW_PRELOAD_PRIO, true ) );
        }

        // add the page before the 'visible series' in preload
        int headRequest = d->visibleItems.first()->pageNumber() - 1;
        if ( headRequest >= 0 )
        {
            PageViewItem * i = d->items[ headRequest ];
            // request the pixmap if not already present
            if ( !i->page()->hasPixmap( PAGEVIEW_ID, i->width(), i->height() ) && i->width() > 0 )
                requestedPixmaps.push_back( new PixmapRequest(
                        PAGEVIEW_ID, i->pageNumber(), i->width(), i->height(), PAGEVIEW_PRELOAD_PRIO, true ) );
        }
    }

    // send requests to the document
    if ( !requestedPixmaps.isEmpty() )
        d->document->requestPixmaps( requestedPixmaps );

    // if this functions was invoked by viewport events, send update to document
    if ( isEvent && nearPageNumber != -1 )
    {
        // determine the document viewport
        DocumentViewport newViewport( nearPageNumber );
        newViewport.rePos.enabled = true;
        newViewport.rePos.normalizedX = focusedX;
        newViewport.rePos.normalizedY = focusedY;
        // set the viewport to other observers
        d->document->setViewport( newViewport , PAGEVIEW_ID);
    }
}

void PageView::slotMoveViewport()
{
    // converge to viewportMoveDest in 1 second
    int diffTime = d->viewportMoveTime.elapsed();
    if ( diffTime >= 667 || !d->viewportMoveActive )
    {
        center( d->viewportMoveDest.x(), d->viewportMoveDest.y() );
        d->viewportMoveTimer->stop();
        d->viewportMoveActive = false;
        slotRequestVisiblePixmaps();
        verticalScrollBar()->setEnabled( true );
        horizontalScrollBar()->setEnabled( true );
        return;
    }

    // move the viewport smoothly (kmplot: p(x)=x+x*(1-x)*(1-x))
    float convergeSpeed = (float)diffTime / 667.0,
          x = ((float)visibleWidth() / 2.0) + contentsX(),
          y = ((float)visibleHeight() / 2.0) + contentsY(),
          diffX = (float)d->viewportMoveDest.x() - x,
          diffY = (float)d->viewportMoveDest.y() - y;
    convergeSpeed *= convergeSpeed * (1.4 - convergeSpeed);
    center( (int)(x + diffX * convergeSpeed),
            (int)(y + diffY * convergeSpeed ) );
}

void PageView::slotAutoScoll()
{
    // the first time create the timer
    if ( !d->autoScrollTimer )
    {
        d->autoScrollTimer = new QTimer( this );
        connect( d->autoScrollTimer, SIGNAL( timeout() ), this, SLOT( slotAutoScoll() ) );
    }

    // if scrollIncrement is zero, stop the timer
    if ( !d->scrollIncrement )
    {
        d->autoScrollTimer->stop();
        return;
    }

    // compute delay between timer ticks and scroll amount per tick
    int index = abs( d->scrollIncrement ) - 1;  // 0..9
    const int scrollDelay[10] =  { 200, 100, 50, 30, 20, 30, 25, 20, 30, 20 };
    const int scrollOffset[10] = {   1,   1,  1,  1,  1,  2,  2,  2,  4,  4 };
    d->autoScrollTimer->changeInterval( scrollDelay[ index ] );
    scrollBy( 0, d->scrollIncrement > 0 ? scrollOffset[ index ] : -scrollOffset[ index ] );
}

void PageView::slotDragScroll()
{
    scrollBy(d->dragScrollVector.x(), d->dragScrollVector.y());
    QPoint p = viewportToContents( mapFromGlobal( QCursor::pos() ) );
    selectionEndPoint( p.x(), p.y() );
}

void PageView::findAheadStop()
{
    d->typeAheadActive = false;
    d->typeAheadString = "";
    d->messageWindow->display( i18n("Find stopped."), PageViewMessage::Find, 1000 );
    // it is needed to grab the keyboard becase people may have Space assigned to a 
    // accel and without grabbing the keyboard you can not vim-search for space
    // because it activates the accel
    releaseKeyboard();
}

void PageView::slotShowWelcome()
{
    // show initial welcome text
    d->messageWindow->display( i18n( "Welcome" ), PageViewMessage::Info, 2000 );
}

void PageView::slotZoom()
{
    setFocus();
    updateZoom( ZoomFixed );
}

void PageView::slotZoomIn()
{
    updateZoom( ZoomIn );
}

void PageView::slotZoomOut()
{
    updateZoom( ZoomOut );
}

void PageView::slotFitToWidthToggled( bool on )
{
    if ( on ) updateZoom( ZoomFitWidth );
}

void PageView::slotFitToPageToggled( bool on )
{
    if ( on ) updateZoom( ZoomFitPage );
}

void PageView::slotFitToTextToggled( bool on )
{
    if ( on ) updateZoom( ZoomFitText );
}

void PageView::slotTwoPagesToggled( bool on )
{
    uint newColumns = on ? 2 : 1;
    if ( KpdfSettings::viewColumns() != newColumns )
    {
        KpdfSettings::setViewColumns( newColumns );
        KpdfSettings::writeConfig();
        if ( d->document->pages() > 0 )
            slotRelayoutPages();
    }
}

void PageView::slotContinuousToggled( bool on )
{
    if ( KpdfSettings::viewContinuous() != on )
    {
        KpdfSettings::setViewContinuous( on );
        KpdfSettings::writeConfig();
        if ( d->document->pages() > 0 )
            slotRelayoutPages();
    }
}

void PageView::slotSetMouseNormal()
{
    d->mouseMode = MouseNormal;
    d->messageWindow->hide();
}

void PageView::slotSetMouseZoom()
{
    d->mouseMode = MouseZoom;
    d->messageWindow->display( i18n( "Select zooming area. Right-click to zoom out." ), PageViewMessage::Info, -1 );
}

void PageView::slotSetMouseSelect()
{
    d->mouseMode = MouseSelect;
    d->messageWindow->display( i18n( "Draw a rectangle around the text/graphics to copy." ), PageViewMessage::Info, -1 );
}

void PageView::slotSetMouseDraw()
{
    d->mouseMode = MouseEdit;
    d->aMouseEdit->setChecked( true );
    d->messageWindow->hide();
}

void PageView::slotScrollUp()
{
    if ( d->scrollIncrement < -9 )
        return;
    d->scrollIncrement--;
    slotAutoScoll();
    setFocus();
}

void PageView::slotScrollDown()
{
    if ( d->scrollIncrement > 9 )
        return;
    d->scrollIncrement++;
    slotAutoScoll();
    setFocus();
}
//END private SLOTS

#include "pageview.moc"
