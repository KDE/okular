/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
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
#include <qpushbutton.h>
#include <qapplication.h>
#include <qclipboard.h>
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

// structure used internally by PageView for data storage
class PageViewPrivate
{
public:
    // the document, pageviewItems and the 'visible cache'
    KPDFDocument * document;
    QValueVector< PageViewItem * > items;
    QValueList< PageViewItem * > visibleItems;

    // view layout (columns and continous in Settings), zoom and mouse
    PageView::ZoomMode zoomMode;
    float zoomFactor;
    PageView::MouseMode mouseMode;
    QPoint mouseGrabPos;
    QPoint mouseStartPos;
    int mouseMidStartY;
    bool mouseOnRect;
    QRect mouseSelectionRect;

    // other stuff
    QTimer * delayTimer;
    QTimer * scrollTimer;
    QTimer * findTimer;
    int scrollIncrement;
    bool dirtyLayout;
    bool typeAheadActivated;
    QString findString;
    bool blockViewport;
    bool blockPixmapsRequest;           // prevent pixmap requests
    PageViewMessage * messageWindow;    // in pageviewutils.h

    // actions
    KToggleAction * aMouseEdit;
    KSelectAction * aZoom;
    KToggleAction * aZoomFitWidth;
    KToggleAction * aZoomFitPage;
    KToggleAction * aZoomFitText;
    KToggleAction * aViewTwoPages;
    KToggleAction * aViewContinous;
};



/* PageView. What's in this file? -> quick overview.
 * Code weight (in rows) and meaning:
 *  160 - constructor and creating actions plus their connected slots (empty stuff)
 *  70  - DocumentObserver inherited methodes (important)
 *  550 - events: mouse, keyboard, drag/drop
 *  170 - slotRelayoutPages: set contents of the scrollview on continous/single modes
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
    d->zoomMode = ZoomFixed;
    d->zoomFactor = 1.0;
    d->mouseMode = MouseNormal;
    d->mouseMidStartY = -1;
    d->mouseOnRect = false;
    d->delayTimer = 0;
    d->scrollTimer = 0;
    d->findTimer = 0;
    d->typeAheadActivated = false;
    d->scrollIncrement = 0;
    d->dirtyLayout = false;
    d->blockViewport = false;
    d->blockPixmapsRequest = false;
    d->messageWindow = new PageViewMessage(this);

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

    // show initial welcome text
    d->messageWindow->display( i18n( "Welcome" ), PageViewMessage::Info, 2000 );

    // set a corner button to resize the view to the page size
//    QPushButton * resizeButton = new QPushButton( viewport() );
//    resizeButton->setPixmap( SmallIcon("crop") );
//    setCornerWidget( resizeButton );
//    resizeButton->setEnabled( false );
    // connect(...);
}

PageView::~PageView()
{
    delete d;
}

void PageView::setupActions( KActionCollection * ac )
{
    // Zoom actions ( higher scales takes lots of memory! )
    d->aZoom = new KSelectAction( i18n( "Zoom" ), "viewmag", 0, this, SLOT( slotZoom() ), ac, "zoom_to" );
    d->aZoom->setEditable( true );
    updateZoomText();

    KStdAction::zoomIn( this, SLOT( slotZoomIn() ), ac, "zoom_in" );

    KStdAction::zoomOut( this, SLOT( slotZoomOut() ), ac, "zoom_out" );

    d->aZoomFitWidth = new KToggleAction( i18n("Fit to Page &Width"), "viewmagfit", 0, ac, "zoom_fit_width" );
    connect( d->aZoomFitWidth, SIGNAL( toggled( bool ) ), SLOT( slotFitToWidthToggled( bool ) ) );

    d->aZoomFitPage = new KToggleAction( i18n("Fit to &Page"), "viewmagfit", 0, ac, "zoom_fit_page" );
    connect( d->aZoomFitPage, SIGNAL( toggled( bool ) ), SLOT( slotFitToPageToggled( bool ) ) );

    d->aZoomFitText = new KToggleAction( i18n("Fit to &Text"), "viewmagfit", 0, ac, "zoom_fit_text" );
    connect( d->aZoomFitText, SIGNAL( toggled( bool ) ), SLOT( slotFitToTextToggled( bool ) ) );

    // View-Layout actions
    d->aViewTwoPages = new KToggleAction( i18n("&Two Pages"), "view_left_right", 0, ac, "view_twopages" );
    connect( d->aViewTwoPages, SIGNAL( toggled( bool ) ), SLOT( slotTwoPagesToggled( bool ) ) );
    d->aViewTwoPages->setChecked( Settings::viewColumns() > 1 );

    d->aViewContinous = new KToggleAction( i18n("&Continous"), "view_text", 0, ac, "view_continous" );
    connect( d->aViewContinous, SIGNAL( toggled( bool ) ), SLOT( slotContinousToggled( bool ) ) );
    d->aViewContinous->setChecked( Settings::viewContinous() );

    // Mouse-Mode actions
    KToggleAction * mn = new KRadioAction( i18n("&Normal"), "mouse", 0, this, SLOT( slotSetMouseNormal() ), ac, "mouse_drag" );
    mn->setExclusiveGroup( "MouseType" );
    mn->setChecked( true );

    KToggleAction * mz = new KRadioAction( i18n("&Zoom Tool"), "viewmag", 0, this, SLOT( slotSetMouseZoom() ), ac, "mouse_zoom" );
    mz->setExclusiveGroup( "MouseType" );

    KToggleAction * mst = new KRadioAction( i18n("&Select"), "frame_edit", 0, this, SLOT( slotSetMouseSelect() ), ac, "mouse_select" );
    mst->setExclusiveGroup( "MouseType" );

    d->aMouseEdit = new KRadioAction( i18n("Draw"), "edit", 0, this, SLOT( slotSetMouseDraw() ), ac, "mouse_draw" );
    d->aMouseEdit->setExclusiveGroup("MouseType");
    d->aMouseEdit->setEnabled( false ); // implement feature before removing this line

    // Other actions
    KAction * su = new KAction( i18n("Scroll Up"), 0, this, SLOT( slotScrollUp() ), ac, "view_scroll_up" );
    su->setShortcut( "Shift+Up" );

    KAction * sd = new KAction( i18n("Scroll Down"), 0, this, SLOT( slotScrollDown() ), ac, "view_scroll_down" );
    sd->setShortcut( "Shift+Down" );
}

bool PageView::canFitPageWidth()
{
    return Settings::viewColumns() != 1 || d->zoomMode != ZoomFitWidth;
}

void PageView::fitPageWidth( int page )
{
    // zoom: Fit Width, columns: 1. setActions + relayout + setPage + update
    d->zoomMode = ZoomFitWidth;
    Settings::setViewColumns( 1 );
    d->aZoomFitWidth->setChecked( true );
    d->aZoomFitPage->setChecked( false );
    d->aZoomFitText->setChecked( false );
    d->aViewTwoPages->setChecked( false );
    viewport()->setUpdatesEnabled( false );
    slotRelayoutPages();
    viewport()->setUpdatesEnabled( true );
    d->document->setViewportPage( page );
    updateZoomText();
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

    // invalidate layout so relayout/repaint will happen on next viewport change
    if ( pageSet.count() > 0 )
        d->dirtyLayout = true;
    else
        resizeContents( 0, 0 );

    // OSD to display pages
    if ( documentChanged && pageSet.count() > 0 && Settings::showOSD() )
        d->messageWindow->display(
            i18n(" Loaded a one-page document.",
                 " Loaded a %n-page document.",
                 pageSet.count() ),
            PageViewMessage::Info, 4000 );
}

void PageView::notifyViewportChanged()
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
        return;
    }

    // relayout in "Single Pages" mode or if a relayout is pending
    d->blockPixmapsRequest = true;
    if ( !Settings::viewContinous() || d->dirtyLayout )
        slotRelayoutPages();

    // restore viewport or use default x-center, v-top alignment
    const QRect & r = item->geometry();
    if ( vp.reCenter.enabled )
    {
        int xCenter = (int)( vp.reCenter.normalizedCenterX * (float)r.width() );
        int yCenter = (int)( vp.reCenter.normalizedCenterY * (float)r.height() );
        center( r.left() + xCenter, r.top() + yCenter );
    }
    else
        center( r.left() + r.width() / 2, r.top() + visibleHeight() / 2 - 10 );
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

void PageView::notifyPageChanged( int pageNumber, int /*changedFlags*/ )
{
    // only handle pixmap changed notifies (the only defined for now)
    //if ( !(changedFlags & DocumentObserver::Pixmap) )
    //    return;

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
    QColor selBlendColor = (selectionRect.width() > 20 || selectionRect.height() > 20) ?
                           palette().active().highlight() : Qt::red;

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

        if ( wantCompositing && Settings::enableCompositing() )
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
            if ( Settings::debugDrawBoundaries() )
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
            if ( Settings::debugDrawBoundaries() )
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
    if ( !d->delayTimer )
    {
        d->delayTimer = new QTimer( this );
        connect( d->delayTimer, SIGNAL( timeout() ), this, SLOT( slotRelayoutPages() ) );
    }
    d->delayTimer->start( 333, true );
}

void PageView::keyPressEvent( QKeyEvent * e )
{
    e->accept();

    // handle 'find as you type' (based on khtml/khtmlview.cpp)
    if( d->typeAheadActivated )
    {
        if( e->key() == Key_BackSpace )
        {
            d->findString = d->findString.left( d->findString.length() - 1 );
            if( !d->findString.isEmpty() )
            {
                findAhead( false );
                d->findTimer->start( 3000, true );
            }
            else
            {
                findTimeout();
                d->document->unHilightPages( false );
            }
            return;
        }
        else if( e->key() == KStdAccel::findNext() )
        {
            // part doesn't get this key event because of the keyboard grab
            d->findTimer->stop(); // restore normal operation during possible messagebox is displayed
            releaseKeyboard();
            if ( d->document->findText() )
                d->messageWindow->display( i18n("Text found: \"%1\".").arg(d->findString.lower()),
                                           PageViewMessage::Find, 3000 );
            d->findTimer->start( 3000, true );
            grabKeyboard();
            return;
        }
        else if( e->key() == Key_Escape || e->key() == Key_Return )
        {
            findTimeout();
            return;
        }
        else if( e->text().isEmpty() == false )
        {
            d->findString += e->text();
            findAhead( true );
            d->findTimer->start( 3000, true );
            return;
        }
    }
    else if( e->key() == '/' && d->document->isOpened() )
    {
        d->findString = QString();
        d->messageWindow->display(i18n("Starting -- find text as you type"), PageViewMessage::Find, 3000);
        d->typeAheadActivated = true;
        if ( !d->findTimer )
        {
            // create the timer on demand
            d->findTimer = new QTimer( this );
            connect( d->findTimer, SIGNAL(timeout()), this, SLOT(findTimeout()) );
        }
        d->findTimer->start( 3000, true );
        grabKeyboard();
        return;
    }

    // move/scroll page by using keys
    switch ( e->key() )
    {
        case Key_Up:
        case Key_PageUp:
            // if in single page mode and at the top of the screen, go to previous page
            if ( Settings::viewContinous() || verticalScrollBar()->value() > verticalScrollBar()->minValue() )
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
                newViewport.reCenter.enabled = true;
                newViewport.reCenter.normalizedCenterY = 1.0;
                d->document->setViewport( newViewport );
            }
            break;
        case Key_Down:
        case Key_PageDown:
            // if in single page mode and at the bottom of the screen, go to next page
            if ( Settings::viewContinous() || verticalScrollBar()->value() < verticalScrollBar()->maxValue() )
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
                newViewport.reCenter.enabled = true;
                newViewport.reCenter.normalizedCenterY = 0.0;
                d->document->setViewport( newViewport );
            }
            break;
        case Key_Left:
            horizontalScrollBar()->subtractLine();
            break;
        case Key_Right:
            horizontalScrollBar()->addLine();
            break;
        case Key_Shift:
        case Key_Control:
            if ( d->scrollTimer )
            {
                if ( d->scrollTimer->isActive() )
                    d->scrollTimer->stop();
                else
                    slotAutoScoll();
                return;
            }
        default:
            e->ignore();
            return;
    }
    // if a known key has been pressed, stop scrolling the page
    if ( d->scrollTimer )
    {
        d->scrollIncrement = 0;
        d->scrollTimer->stop();
    }
}

void PageView::findTimeout()
{
    d->typeAheadActivated = false;
    d->findString = "";
    d->messageWindow->display(i18n("Find stopped."),PageViewMessage::Find,1000);
    releaseKeyboard();
}

void PageView::findAhead(bool increase)
{
    if (!increase) 
        d->document->setViewportPage(0);
    QString status;
    d->document->unHilightPages(false);
    bool found = d->document->findText(d->findString, false, true);
    if(found)
        status = i18n("Text found: \"%1\".");
    else 
        status = i18n("Text not found: \"%1\".");
    d->messageWindow->display(status.arg(d->findString.lower()),
        found ? PageViewMessage::Find : PageViewMessage::Warning, 4000);
}


void PageView::contentsMouseMoveEvent( QMouseEvent * e )
{
    // if holding mouse mid button, perform zoom
    if ( (e->state() & MidButton) && d->mouseMidStartY > 0 )
    {
        int deltaY = d->mouseMidStartY - e->globalPos().y();
        d->mouseMidStartY = e->globalPos().y();
        d->zoomFactor *= ( 1.0 + ( (double)deltaY / 500.0 ) );
        updateZoom( ZoomRefreshCurrent );
        // uncomment following line to force a complete redraw
        viewport()->repaint( false );
        return;
    }

    bool leftButton = e->state() & LeftButton;
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
                    // if the page was scrolling, stop it
                    if ( d->scrollTimer )
                    {
                        d->scrollIncrement = 0;
                        d->scrollTimer->stop();
                    }
                }
            }
            else // only hovering the page
            {
                updateCursor( e->pos() );
            }
            break;

        case MouseZoom:
        case MouseSelect:
            // set second corner of selection in selection pageItem
            if ( leftButton && !d->mouseSelectionRect.isNull() )
                selectionEndPoint( e->x(), e->y() );
            break;

        case MouseEdit:      // ? update graphics ?
            break;
    }
}

void PageView::contentsMousePressEvent( QMouseEvent * e )
{
    // if pressing mid mouse button while not doing other things, begin 'comtinous zoom' mode
    if ( (e->button() & MidButton) && d->mouseSelectionRect.isNull() )
    {
        d->mouseMidStartY = e->globalPos().y();
        setCursor( sizeVerCursor );
        return;
    }

    // handle mode dependant mouse press actions
    bool leftButton = e->button() & LeftButton;
    switch ( d->mouseMode )
    {
        case MouseNormal:    // drag start / click / link following
            if ( leftButton )
            {
                d->mouseStartPos = e->globalPos();
                d->mouseGrabPos = d->mouseOnRect ? QPoint() : d->mouseStartPos;
                if ( !d->mouseOnRect )
                    setCursor( sizeAllCursor );
            }
            break;

        case MouseZoom:
        case MouseSelect:   // set first corner of the selection rect
            if ( leftButton )
                selectionStart( e->x(), e->y(), false );
            break;

        case MouseEdit:      // ? place the beginning of [tool] ?
            break;
    }
}

void PageView::contentsMouseReleaseEvent( QMouseEvent * e )
{
    // handle mode indepent mid buttom zoom
    if ( (e->state() & MidButton) && d->mouseMidStartY > 0 )
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
        case MouseNormal:{  // do Follow Link or Display RMB
            // return the cursor to its normal state after dragging
            if (cursor().shape() == Qt::SizeAllCursor) updateCursor( e->pos() );

            PageViewItem * pageItem = pickItemOnPoint( e->x(), e->y() );

            // avoid the situation in where you click on a "row" that has a link but you are not over it
            // drag a bit and move the mouse left to place it over the link while dragging
            // release the button and BOOM you get the link followed
            if ( leftButton && pageItem && d->mouseStartPos == e->globalPos())
            {
                const KPDFPageRect * rect = pageItem->page()->getRect(
                    e->x() - pageItem->geometry().left(),
                    e->y() - pageItem->geometry().top()
                );
                if ( rect )
                {
                    // release over a link
                    if ( rect->pointerType() == KPDFPageRect::Link )
                    {
                        const KPDFLink * link = static_cast< const KPDFLink * >( rect->pointer() );
                        d->document->processLink( link );
                    }
                    // release over an image
                    if ( rect->pointerType() == KPDFPageRect::Image )
                    {
                    }
                }
                else
                {
                    // mouse not moved since press, so we have a click. select the page.
                    d->document->setViewportPage( pageItem->pageNumber(), PAGEVIEW_ID );
                }
            }
            else if ( rightButton )
            {
                if (pageItem) emit rightClick(pageItem->page(), e->globalPos());
                else emit rightClick(0, e->globalPos());
            }
            // reset start position
            d->mouseStartPos = QPoint();
            }break;

        case MouseZoom:     // do ZOOM
            if ( leftButton && !d->mouseSelectionRect.isNull() )
            {
                QRect selRect = d->mouseSelectionRect.normalize();
                if ( selRect.width() < 4 || selRect.height() < 4 )
                    break;

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
                return;
            }
            // if in ZoomRect mode, right click zooms out
            else if ( rightButton )
            {
                updateZoom( ZoomOut );
                return;
            }
            break;

        case MouseSelect:{  // do SELECT
            if ( !leftButton || d->mouseSelectionRect.isNull() )
                break;

            QRect selectionRect = d->mouseSelectionRect.normalize();
            if ( selectionRect.width() < 5 || selectionRect.height() < 5 )
            {
                selectionClear();
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
                    // grab text
                    QRect relativeRect = selectionRect.intersect( itemRect );
                    relativeRect.moveBy( -itemRect.left(), -itemRect.top() );
                    selectedText += kpdfPage->getTextInRect( relativeRect, item->zoomFactor() );
                }
            }

            // popup that ask to copy:text and copy/save:image
            KPopupMenu menu( this );
            if ( !selectedText.isEmpty() )
            {
                menu.insertTitle( i18n( "Text (1 character)", "Text (%n characters)", selectedText.length() ) );
                menu.insertItem( SmallIcon("editcopy"), i18n( "Copy to Clipboard" ), 1 );
            }
            menu.insertTitle( i18n( "Image (%1 by %2 pixels)" ).arg( selectionRect.width() ).arg( selectionRect.height() ) );
            menu.insertItem( SmallIcon("image"), i18n( "Copy to Clipboard" ), 2 );
            menu.insertItem( SmallIcon("filesave"), i18n( "Save to File..." ), 3 );
            int choice = menu.exec( e->globalPos() );
            // IMAGE operation choosen
            if ( choice > 1 )
            {
                // renders page into a pixmap
                QPixmap copyPix( selectionRect.width(), selectionRect.height() );
                QPainter copyPainter( &copyPix );
                copyPainter.translate( -selectionRect.left(), -selectionRect.top() );
                paintItems( &copyPainter, selectionRect );

                if ( choice == 2 )
                {
                    // [2] copy pixmap to clipboard
                    QClipboard *cb = QApplication::clipboard();
                    cb->setPixmap( copyPix, QClipboard::Clipboard );
                    if ( cb->supportsSelection() )
                        cb->setPixmap( copyPix, QClipboard::Selection );
                    d->messageWindow->display( i18n( "Image [%1x%2] copied to clipboard." ).arg( copyPix.width() ).arg( copyPix.height() ) );
                }
                else
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
            else if ( choice == 1 )
            {
                QClipboard *cb = QApplication::clipboard();
                cb->setText( selectedText, QClipboard::Clipboard );
                if ( cb->supportsSelection() )
                    cb->setText( selectedText, QClipboard::Selection );
            }

            // clear widget selection and invalidate rect
            selectionClear();
            }break;

        case MouseEdit:      // ? apply [tool] ?
            break;
    }
}

void PageView::wheelEvent( QWheelEvent *e )
{
    int delta = e->delta(),
        vScroll = verticalScrollBar()->value();
    e->accept();
    if ( (e->state() & ControlButton) == ControlButton ) {
        if ( e->delta() < 0 )
            slotZoomOut();
        else
            slotZoomIn();
    }
    else if ( delta <= -120 && !Settings::viewContinous() && vScroll == verticalScrollBar()->maxValue() )
    {
        // go to next page
        if ( d->document->currentPage() < d->items.count() - 1 )
        {
            // more optmized than document->setNextPage and then move view to top
            DocumentViewport newViewport = d->document->viewport();
            newViewport.pageNumber += 1;
            newViewport.reCenter.enabled = true;
            newViewport.reCenter.normalizedCenterY = 0.0;
            d->document->setViewport( newViewport );
        }
    }
    else if ( delta >= 120 && !Settings::viewContinous() && vScroll == verticalScrollBar()->minValue() )
    {
        // go to prev page
        if ( d->document->currentPage() > 0 )
        {
            // more optmized than document->setPrevPage and then move view to bottom
            DocumentViewport newViewport = d->document->viewport();
            newViewport.pageNumber -= 1;
            newViewport.reCenter.enabled = true;
            newViewport.reCenter.normalizedCenterY = 1.0;
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
                        PagePainter::EnhanceImages | PagePainter::Highlight;
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
    }
    else if ( d->zoomMode == ZoomFitPage )
    {
        double scaleW = (double)colWidth / (double)width;
        double scaleH = (double)rowHeight / (double)height;
        zoom = QMIN( scaleW, scaleH );
        item->setWHZ( (int)(zoom * width), (int)(zoom * height), zoom );
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

void PageView::selectionStart( int x, int y, bool /*aboveAll*/ )
{
    d->mouseSelectionRect.setRect( x, y, 1, 1 );
    // ensures page doesn't scroll
    if ( d->scrollTimer )
    {
        d->scrollIncrement = 0;
        d->scrollTimer->stop();
    }
}

void PageView::selectionEndPoint( int x, int y )
{
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
    if ( pageItem )
    {
        int pageX = p.x() - pageItem->geometry().left(),
            pageY = p.y() - pageItem->geometry().top();

        // check if over a KPDFPageRect
        bool onLink = pageItem->page()->hasLink( pageX, pageY );
        d->mouseOnRect = onLink;
        if ( onLink )
            setCursor( pointingHandCursor );
        else
            setCursor( arrowCursor );
    }
    else
    {
        // if there's no page over the cursor and we were showing the pointingHandCursor
        // go back to the normal one
        d->mouseOnRect = false;
        setCursor( arrowCursor );
    }
}

//BEGIN private SLOTS
void PageView::slotRelayoutPages()
// called by: notifySetup, viewportResizeEvent, slotTwoPagesToggled, slotContinousToggled, updateZoom
{
    // set an empty container if we have no pages
    int pageCount = d->items.count();
    if ( pageCount < 1 )
    {
        resizeContents( 0, 0 );
        return;
    }

    // common iterator used in this method and viewport parameters
    QValueVector< PageViewItem * >::iterator iIt, iEnd = d->items.end();
    int viewportWidth = visibleWidth(),
        viewportHeight = visibleHeight(),
        fullWidth = 0,
        fullHeight = 0;
    QRect viewportRect( contentsX(), contentsY(), viewportWidth, viewportHeight );

    // set all items geometry and resize contents. handle 'continous' and 'single' modes separately
    if ( Settings::viewContinous() )
    {
        // Here we find out column's width and row's height to compute a table
        // so we can place widgets 'centered in virtual cells'.
        int nCols = Settings::viewColumns(),
            nRows = (int)ceilf( (float)pageCount / (float)nCols ),
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
    else // viewContinous is FALSE
    {
        PageViewItem * currentItem = d->items[ QMAX( 0, (int)d->document->currentPage() ) ];

        // setup varialbles for a 1(row) x N(columns) grid
        int nCols = Settings::viewColumns(),
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

        // this can cause a little overhead
        slotRequestVisiblePixmaps();
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
                const QRect & geometry = d->items[ vp.pageNumber ]->geometry();
                double nX = vp.reCenter.enabled ? vp.reCenter.normalizedCenterX : 0.5,
                    nY = vp.reCenter.enabled ? vp.reCenter.normalizedCenterY : 0.0;
                center( geometry.left() + ROUND( nX * (double)geometry.width() ),
                        geometry.top() + ROUND( nY * (double)geometry.height() ) );
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
    if ( d->blockPixmapsRequest )
        return;

    // precalc view limits for intersecting with page coords inside the lOOp
    bool isEvent = newLeft != -1 && newTop != -1 && !d->blockViewport;
    QRect viewportRect( isEvent ? newLeft : contentsX(),
                        isEvent ? newTop : contentsY(),
                        visibleWidth(), visibleHeight() );

    // some variables used to determine the viewport
    int nearPageNumber = -1,
        viewportCenterX = (viewportRect.left() + viewportRect.right()) / 2,
        viewportCenterY = (viewportRect.top() + viewportRect.bottom()) / 2;
    double focusedX = 0.5,
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
                focusedX = (double)( viewportCenterX - geometry.left() ) / (double)geometry.width();
                focusedY = (double)( viewportCenterY - geometry.top() ) / (double)geometry.height();
            }
        }
    }

    // if preloading is enabled, add the pages before and after in preloading
    if ( !d->visibleItems.isEmpty() &&
         Settings::memoryLevel() != Settings::EnumMemoryLevel::Low &&
         Settings::enableThreading() )
    {
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
    }

    // send requests to the document
    if ( !requestedPixmaps.isEmpty() )
        d->document->requestPixmaps( requestedPixmaps );

    // if this functions was invoked by viewport events
    if ( isEvent && nearPageNumber != -1 )
    {
        // determine the document viewport
        DocumentViewport newViewport( nearPageNumber );
        newViewport.reCenter.enabled = true;
        newViewport.reCenter.normalizedCenterX = focusedX;
        newViewport.reCenter.normalizedCenterY = focusedY;
        // set the viewport to other observers
        d->document->setViewport( newViewport , PAGEVIEW_ID);
    }
}

void PageView::slotAutoScoll()
{
    // the first time create the timer
    if ( !d->scrollTimer )
    {
        d->scrollTimer = new QTimer( this );
        connect( d->scrollTimer, SIGNAL( timeout() ), this, SLOT( slotAutoScoll() ) );
    }

    // if scrollIncrement is zero, stop the timer
    if ( !d->scrollIncrement )
    {
        d->scrollTimer->stop();
        return;
    }

    // compute delay between timer ticks and scroll amount per tick
    int index = abs( d->scrollIncrement ) - 1;  // 0..9
    const int scrollDelay[10] =  { 200, 100, 50, 30, 20, 30, 25, 20, 30, 20 };
    const int scrollOffset[10] = {   1,   1,  1,  1,  1,  2,  2,  2,  4,  4 };
    d->scrollTimer->changeInterval( scrollDelay[ index ] );
    scrollBy( 0, d->scrollIncrement > 0 ? scrollOffset[ index ] : -scrollOffset[ index ] );
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
    if ( Settings::viewColumns() != newColumns )
    {
        Settings::setViewColumns( newColumns );
        if ( d->document->pages() > 0 )
            slotRelayoutPages();
    }
}

void PageView::slotContinousToggled( bool on )
{
    if ( Settings::viewContinous() != on )
    {
        Settings::setViewContinous( on );
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
