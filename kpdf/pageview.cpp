/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   With portions of code from kpdf_pagewidget.cc by:                     *
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

#include <qpainter.h>
#include <qtimer.h>
#include <qpushbutton.h>
#include <qapplication.h>
#include <qclipboard.h>

#include <kiconloader.h>
#include <kurldrag.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kpopupmenu.h>
#include <klocale.h>

#include <math.h>
#include <stdlib.h>

#include "pageview.h"
#include "pageviewutils.h"
#include "page.h"
#include "settings.h"


// structure used internally by PageView for data storage
class PageViewPrivate
{
public:
    // the document, current page and pages indices vector
    KPDFDocument * document;
    PageViewItem * activeItem; //equal to pages[vectorIndex]
    QValueVector< PageViewItem * > pages;
    int vectorIndex;

    // view layout (columns and continous in Settings), zoom and mouse
    PageView::ZoomMode zoomMode;
    float zoomFactor;
    PageView::MouseMode mouseMode;
    QPoint mouseGrabPos;
    QPoint mouseStartPos;
    bool mouseOnLink;
    bool mouseOnActiveRect;
    QRect mouseSelectionRect;

    // other stuff
    QTimer * delayTimer;
    QTimer * scrollTimer;
    int scrollIncrement;
    bool dirtyLayout;
    PageViewOverlay * overlayWindow;    //in pageviewutils.h
    PageViewMessage * messageWindow;    //in pageviewutils.h

    // actions
    KToggleAction * aMouseEdit;
    KSelectAction * aZoom;
    KToggleAction * aZoomFitWidth;
    KToggleAction * aZoomFitPage;
    KToggleAction * aZoomFitText;
    KToggleAction * aZoomFitRect;
    KToggleAction * aViewTwoPages;
    KToggleAction * aViewContinous;
};



/* PageView. What's in this file? -> quick overview.
 * Code weight (in rows) and meaning:
 *  160 - constructor and creating actions plus their connected slots (empty stuff)
 *  70  - DocumentObserver inherited methodes (important)
 *  200 - events: mouse, keyboard, drag/drop
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
    d->activeItem = 0;
    d->vectorIndex = -1;
    d->zoomMode = ZoomFixed;
    d->zoomFactor = 1.0;
    d->mouseMode = MouseNormal;
    d->mouseOnLink = false;
    d->mouseOnActiveRect = false;
    d->delayTimer = 0;
    d->scrollTimer = 0;
    d->scrollIncrement = 0;
    d->dirtyLayout = false;
    d->overlayWindow = 0;
    d->messageWindow = new PageViewMessage(this);

    // widget setup: setup focus, accept drops and track mouse
    viewport()->setFocusProxy( this );
    viewport()->setFocusPolicy( StrongFocus );
    viewport()->setPaletteBackgroundColor( Qt::gray );
    setResizePolicy( Manual );
    setAcceptDrops( true );
    setDragAutoScroll( false );
    viewport()->setMouseTracking( true );

    // conntect the padding of the viewport to pixmaps requests
    connect( this, SIGNAL(contentsMoving(int, int)), this, SLOT(slotRequestVisiblePixmaps(int, int)) );

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

    d->aZoomFitRect = new KToggleAction( i18n("Zoom to Rect"), "viewmag", 0, ac, "zoom_fit_rect" );
    connect( d->aZoomFitRect, SIGNAL( toggled( bool ) ), SLOT( slotFitToRectToggled( bool ) ) );

    // View-Layout actions
    d->aViewTwoPages = new KToggleAction( i18n("Two Pages"), "view_left_right", 0, ac, "view_twopages" );
    connect( d->aViewTwoPages, SIGNAL( toggled( bool ) ), SLOT( slotTwoPagesToggled( bool ) ) );
    d->aViewTwoPages->setChecked( Settings::viewColumns() > 1 );

    d->aViewContinous = new KToggleAction( i18n("Continous"), "view_text", 0, ac, "view_continous" );
    connect( d->aViewContinous, SIGNAL( toggled( bool ) ), SLOT( slotContinousToggled( bool ) ) );
    d->aViewContinous->setChecked( Settings::viewContinous() );

    // Mouse-Mode actions
    KToggleAction * mn = new KRadioAction( i18n("Normal"), "mouse", 0, this, SLOT( slotSetMouseNormal() ), ac, "mouse_drag" );
    mn->setExclusiveGroup("MouseType");
    mn->setChecked( true );

    KToggleAction *ms = new KRadioAction( i18n("Select"), "frame_edit", 0, this, SLOT( slotSetMouseSelect() ), ac, "mouse_select" );
    ms->setExclusiveGroup("MouseType");

    d->aMouseEdit = new KRadioAction( i18n("Draw"), "edit", 0, this, SLOT( slotSetMouseDraw() ), ac, "mouse_draw" );
    d->aMouseEdit->setExclusiveGroup("MouseType");
    d->aMouseEdit->setEnabled( false ); // implement feature before removing this line

    // Other actions
    KAction * su = new KAction( i18n("Scroll Up"), 0, this, SLOT( slotScrollUp() ), ac, "view_scroll_up" );
    su->setShortcut( "Shift+Up" );

    KAction * sd = new KAction( i18n("Scroll Down"), 0, this, SLOT( slotScrollDown() ), ac, "view_scroll_down" );
    sd->setShortcut( "Shift+Down" );
}


//BEGIN KPDFDocumentObserver inherited methods
void PageView::pageSetup( const QValueVector<KPDFPage*> & pageSet, bool documentChanged )
{
    // reuse current pages if nothing new
    if ( ( pageSet.count() == d->pages.count() ) && !documentChanged )
    {
        int count = pageSet.count();
        for ( int i = 0; (i < count) && !documentChanged; i++ )
            if ( (int)pageSet[i]->number() != d->pages[i]->pageNumber() )
                documentChanged = true;
        if ( !documentChanged )
            return;
    }

    // delete all widgets (one for each page in pageSet)
    QValueVector< PageViewItem * >::iterator dIt = d->pages.begin(), dEnd = d->pages.end();
    for ( ; dIt != dEnd; ++dIt )
        delete *dIt;
    d->pages.clear();
    d->activeItem = 0;

    // create children widgets
    QValueVector< KPDFPage * >::const_iterator setIt = pageSet.begin(), setEnd = pageSet.end();
    for ( ; setIt != setEnd; ++setIt )
        d->pages.push_back( new PageViewItem( *setIt ) );

    // invalidate layout
    d->dirtyLayout = true;

    // OSD to display pages
    if ( documentChanged && !Settings::hideOSD() )
        d->messageWindow->display(
            i18n(" Loaded a %1 pages document." ).arg( pageSet.count() ),
            PageViewMessage::Info, 4000 );
}

void PageView::pageSetCurrent( int pageNumber, const QRect & /*viewport*/ )
{
    // select next page
    d->vectorIndex = 0;
    d->activeItem = 0;
    QValueVector< PageViewItem * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
    for ( ; pIt != pEnd; ++pIt )
    {
        if ( (*pIt)->pageNumber() == pageNumber )
        {
            d->activeItem = *pIt;
            break;
        }
        d->vectorIndex ++;
    }
    if ( !d->activeItem )
        return;

    // relayout in "Single Pages" mode or if a relayout is pending
    if ( !Settings::viewContinous() || d->dirtyLayout )
        slotRelayoutPages();

    // center the view to see the selected page
    // FIXME take care of viewport
    const QRect & r = d->activeItem->geometry();
    center( r.left() + r.width() / 2, r.top() + visibleHeight() / 2 - 10 );
    slotRequestVisiblePixmaps();

    // update zoom text if in a ZoomFit/* zoom mode
    if ( d->zoomMode != ZoomFixed )
        updateZoomText();
}

void PageView::notifyPixmapChanged( int pageNumber )
{
    QValueVector< PageViewItem * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
    for ( ; pIt != pEnd; ++pIt )
        if ( (*pIt)->pageNumber() == pageNumber )
        {
            updateContents( (*pIt)->geometry() );
            break;
        }
}
//END KPDFDocumentObserver inherited methods

//BEGIN widget events
#include <kdebug.h>
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

    // Iterate over the regions. This optimizes a lot the cases in which
    // SUMj( Region[j].area ) is less than Region.boundingRect.area )
    QMemArray<QRect> allRects = pe->region().rects();
    int numRects = allRects.count();
    // TODO: add a check to see wether to use area subdivision or not.
    //kdDebug() << "painting " << numRects << " rects" << endl;
    for ( uint i = 0; i < numRects; i++ )
    {
        //QRegion remaining( r );
        contentsRect = allRects[i].intersect( viewportRect );
        contentsRect.moveBy( contentsX(), contentsY() );

        if ( Settings::tempUseComposting() )
        {
            // create pixmap and open a painter over it
            QPixmap doubleBuffer( contentsRect.size() );
            QPainter pixmapPainter( &doubleBuffer );
            pixmapPainter.translate( -contentsRect.left(), -contentsRect.top() );

            // gfx operations on pixmap (rect {left,top} is pixmap {0,0})
            // 1) clear bg
            pixmapPainter.fillRect( contentsRect, Qt::gray );
            // 2) paint items
            paintItems( &pixmapPainter, contentsRect );
            // 3) pixmap manipulated areas
            // 4) paint transparent selections
            if ( !d->mouseSelectionRect.isNull() )
            {
                pixmapPainter.save();
                pixmapPainter.setPen( palette().active().highlight().dark(110) );
                pixmapPainter.setBrush( QBrush( palette().active().highlight(), Qt::Dense4Pattern ) );
                pixmapPainter.drawRect( d->mouseSelectionRect.normalize() );
                pixmapPainter.restore();
            }
            // 5) paint overlays
            if ( Settings::tempDrawBoundaries() )
            {
                pixmapPainter.setPen( Qt::blue );
                pixmapPainter.drawRect( contentsRect );
            }

            // finish painting and draw contents
            pixmapPainter.end();
            screenPainter.drawPixmap( contentsRect.left(), contentsRect.top(), doubleBuffer );
        }
        else    // not using COMPOSTING
        {
            // 1) clear bg
            screenPainter.fillRect( contentsRect, Qt::gray );
            // 2) paint items
            paintItems( &screenPainter, contentsRect );
            // 4) paint opaque selections
            if ( !d->mouseSelectionRect.isNull() )
            {
                screenPainter.setPen( palette().active().highlight().dark(110) );
                screenPainter.drawRect( d->mouseSelectionRect.normalize() );
            }
            // 5) paint overlays
            if ( Settings::tempDrawBoundaries() )
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
    d->delayTimer->start( 400, true );
}

void PageView::keyPressEvent( QKeyEvent * e )
{
    e->accept();
    // move/scroll page by using keys
    switch ( e->key() )
    {
        case Key_Up:
            if ( Settings::viewContinous() || verticalScrollBar()->value() > verticalScrollBar()->minValue() )
                verticalScrollBar()->subtractLine();
            // if in single page mode and at the top of the screen, go to previous page
            else if ( d->vectorIndex > 0 )
                d->document->slotSetCurrentPage( d->pages[ d->vectorIndex - 1 ]->pageNumber() );
            break;
        case Key_Down:
            if ( Settings::viewContinous() || verticalScrollBar()->value() < verticalScrollBar()->maxValue() )
                verticalScrollBar()->addLine();
            // if in single page mode and at the bottom of the screen, go to next page
            else if ( d->vectorIndex < (int)d->pages.count() - 1 )
                d->document->slotSetCurrentPage( d->pages[ d->vectorIndex + 1 ]->pageNumber() );
            break;
        case Key_Left:
            horizontalScrollBar()->subtractLine();
            break;
        case Key_Right:
            horizontalScrollBar()->addLine();
            break;
        case Key_PageUp:
            verticalScrollBar()->subtractPage();
            break;
        case Key_PageDown:
            verticalScrollBar()->addPage();
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

void PageView::contentsMouseMoveEvent( QMouseEvent * e )
{
    bool leftButton = e->state() & LeftButton;

    // handle 'Zoom To Area', in every mouse mode
    if ( leftButton && d->zoomMode == ZoomRect && !d->mouseStartPos.isNull() )
    {
        // create zooming a window (overlay mode)
        if ( !d->overlayWindow )
        {
            d->overlayWindow = new PageViewOverlay( viewport(), PageViewOverlay::Zoom );
            d->overlayWindow->setBeginCorner( d->mouseStartPos.x() - contentsX(), d->mouseStartPos.y() - contentsY() );
        }

        // set rect's 2nd corner
        d->overlayWindow->setEndCorner( e->x() - contentsX(), e->y() - contentsY() );
        return;
    }

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
            else
            {
/*                // detect the underlaying page (if present)
                PageViewItem * pageItem = pickItemOnPoint( e->x(), e->y() );
                if ( PageViewItem )
                {
                    int pageX = e->x() - childX( pageItem ),
                        pageY = e->y() - childY( pageItem );

                    // check if over a KPDFActiveRect
                    bool onActiveRect = pageItem->page()->hasActiveRect( pageX, pageY );
                    if ( onActiveRect != d->mouseOnActiveRect )
                    {
                        d->mouseOnActiveRect = onActiveRect;
                        setCursor( onActiveRect ? pointingHandCursor : arrowCursor );
                    }

                    // check if over a KPDFLink
                    bool onLink = pageItem->page()->hasLink( pageX, pageY );
                    if ( onLink != d->mouseOnLink )
                    {
                        d->mouseOnLink = onLink;
                        setCursor( onLink ? pointingHandCursor : arrowCursor );
                    }
                }
*/            }
            break;

        case MouseSelection:
            // set second corner of selection in selection pageItem
            if ( leftButton && d->activeItem && !d->mouseSelectionRect.isNull() )
            {
                const QRect & itemRect = d->activeItem->geometry();
                // clip selection inside the page
                int x = QMAX( QMIN( e->x(), itemRect.right() ), itemRect.left() ),
                    y = QMAX( QMIN( e->y(), itemRect.bottom() ), itemRect.top() );
                // if selection changed update rect
                if ( d->mouseSelectionRect.right() != x || d->mouseSelectionRect.bottom() != y )
                {
                    // Send only incremental paint events!
                    QRect oldRect = d->mouseSelectionRect.normalize();
                    d->mouseSelectionRect.setRight( x );
                    d->mouseSelectionRect.setBottom( y );
                    QRect newRect = d->mouseSelectionRect.normalize();
                    // (enqueue rects 1) areas to clear with bg color
                    QMemArray<QRect> transparentRects = QRegion( oldRect ).subtract( newRect ).rects();
                    for ( uint i = 0; i < transparentRects.count(); i++ )
                        updateContents( transparentRects[i] );
                    // (enqueue rects 2) new opaque areas 
                    oldRect.addCoords( 1, 1, -1, -1 );
                    QMemArray<QRect> opaqueRects = QRegion( newRect ).subtract( oldRect ).rects();
                    for ( uint i = 0; i < opaqueRects.count(); i++ )
                        updateContents( opaqueRects[i] );
                }
            }
            break;

        case MouseEdit:      // ? update graphics ?
            break;
    }
}

void PageView::contentsMousePressEvent( QMouseEvent * e )
{
    bool leftButton = e->button() & LeftButton;

    // handle 'Zoom To Area', in every mouse mode
    if ( leftButton && d->zoomMode == ZoomRect )
    {
        d->mouseStartPos = e->pos();
        d->mouseGrabPos = QPoint();
        return;
    }

    // handle mode dependant mouse press actions
    switch ( d->mouseMode )
    {
        case MouseNormal:    // drag start / click / link following
            if ( leftButton )
            {
                d->mouseStartPos = e->globalPos();
                d->mouseGrabPos = d->mouseOnLink ? QPoint() : d->mouseStartPos;
                if ( !d->mouseOnLink )
                    setCursor( sizeAllCursor );
            }
            else if ( e->button() & RightButton )
                emit rightClick();
            break;

        case MouseSelection: // set first corner of the selection rect
            if ( leftButton )
            {
                PageViewItem * item = pickItemOnPoint( e->x(), e->y() );
                if ( item )
                {
                    // pick current page as the active one
                    d->activeItem = item;
                    d->mouseSelectionRect.setRect( e->x(), e->y(), 1, 1 );
                    // ensures page doesn't scroll
                    if ( d->scrollTimer )
                    {
                        d->scrollIncrement = 0;
                        d->scrollTimer->stop();
                    }
                }
            }
            break;

        case MouseEdit:      // ? place the beginning of [tool] ?
            break;
    }
}

void PageView::contentsMouseReleaseEvent( QMouseEvent * e )
{
    bool leftButton = e->button() & LeftButton,
         rightButton = e->button() & RightButton;

    // handle 'Zoom To Area', in every mouse mode
    if ( leftButton && d->overlayWindow )
    {
        // find out new zoom ratio
        QRect selRect = d->overlayWindow->selectedRect();
        float zoom = QMIN( (float)visibleWidth() / (float)selRect.width(),
                            (float)visibleHeight() / (float)selRect.height() );

        // get normalized view center (relative to the contentsRect)
        // coeffs (1.0 and 1.5) are for correcting the asymmetic page border
        // that makes the page not perfectly centered on the viewport
        double nX = ( contentsX() - 1.0 + selRect.left() + (double)selRect.width() / 2.0 ) / (double)contentsWidth();
        double nY = ( contentsY() - 1.5 + selRect.top() + (double)selRect.height() / 2.0 ) / (double)contentsHeight();

        // zoom up to 400%
        if ( d->zoomFactor <= 4.0 || zoom <= 1.0 )
        {
            d->zoomFactor *= zoom;
            if ( d->zoomFactor > 4.0 )
                d->zoomFactor = 4.0;
            updateZoom( ZoomRefreshCurrent );
        }

        // recenter view
        center( (int)(nX * contentsWidth()), (int)(nY * contentsHeight()) );

        // hide message box and delete overlay window
        d->messageWindow->hide();
        delete d->overlayWindow;
        d->overlayWindow = 0;

        // reset start position
        d->mouseStartPos = QPoint();
        return;
    }
    // if in ZoomRect mode, right click zooms out
    if ( rightButton && d->zoomMode == ZoomRect )
    {
        updateZoom( ZoomOut );
        updateZoom( ZoomRect );
        return;
    }

    PageViewItem * pageItem = pickItemOnPoint( e->x(), e->y() );
    switch ( d->mouseMode )
    {
        case MouseNormal:
            setCursor( arrowCursor );
            if ( leftButton && pageItem )
            {
                if ( d->mouseOnLink )
                {
                    // activate link
                    int linkX = e->x() - pageItem->geometry().left(),
                        linkY = e->y() - pageItem->geometry().top();
                    d->document->slotProcessLink( pageItem->page()->getLink( linkX, linkY ) );
                }
                else
                {
                    // mouse not moved since press, so we have a click. select the page.
                    if ( e->globalPos() == d->mouseStartPos )
                        d->document->slotSetCurrentPage( pageItem->pageNumber() );
                }
            }
            else if ( rightButton && pageItem )
            {
                // if over a page display a popup menu
                const KPDFPage * kpdfPage = pageItem->page();
                KPopupMenu * m_popup = new KPopupMenu( this, "rmb popup" );
                m_popup->insertTitle( i18n( "Page %1" ).arg( kpdfPage->number() + 1 ) );
                if ( kpdfPage->isBookmarked() )
                    m_popup->insertItem( SmallIcon("bookmark"), i18n("Remove Bookmark"), 1 );
                else
                    m_popup->insertItem( SmallIcon("bookmark_add"), i18n("Add Bookmark"), 1 );
                m_popup->insertItem( SmallIcon("viewmagfit"), i18n("Fit Page"), 2 );
                m_popup->insertItem( SmallIcon("pencil"), i18n("Edit"), 3 );
                switch ( m_popup->exec(e->globalPos()) )
                {
                    case 1:
                        d->document->slotBookmarkPage( kpdfPage->number(), !kpdfPage->isBookmarked() );
                        break;
                    case 2: // FiXME less hackish, please!
                        d->aZoomFitWidth->setChecked( true );
                        updateZoom( ZoomFitWidth );
                        d->aViewTwoPages->setChecked( false );
                        slotTwoPagesToggled( false );
                        d->document->slotSetCurrentPage( kpdfPage->number() );
                        break;
                    case 3: // ToDO switch to edit mode
                        slotSetMouseDraw();
                        break;
                }
                delete m_popup;
            }
            // reset start position
            d->mouseStartPos = QPoint();
            break;

        case MouseSelection:
            if ( leftButton && d->activeItem && !d->mouseSelectionRect.isNull() )
            {
                // request the textpage if there isn't one
                const KPDFPage * kpdfPage = d->activeItem->page();
                if ( !kpdfPage->hasSearchPage() )
                    d->document->requestTextPage( kpdfPage->number() );

                // copy text into the clipboard
                QClipboard *cb = QApplication::clipboard();
                QRect relativeRect = d->mouseSelectionRect.normalize();
                relativeRect.moveBy( -d->activeItem->geometry().left(), -d->activeItem->geometry().top() );
                const QString & selection = kpdfPage->getTextInRect( relativeRect, d->activeItem->zoomFactor() );

                cb->setText( selection, QClipboard::Clipboard );
                if ( cb->supportsSelection() )
                    cb->setText( selection, QClipboard::Selection );

                // clear widget selection and invalidate rect
                if ( !d->mouseSelectionRect.isNull() )
                    updateContents( d->mouseSelectionRect.normalize() );
                d->mouseSelectionRect.setCoords( 0, 0, -1, -1 );
            }
            break;

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
        if ( d->vectorIndex < (int)d->pages.count() - 1 )
            d->document->slotSetCurrentPage( d->pages[ d->vectorIndex + 1 ]->pageNumber() );
    }
    else if ( delta >= 120 && !Settings::viewContinous() && vScroll == verticalScrollBar()->minValue() )
    {
        // go to prev page
        if ( d->vectorIndex > 0 )
            d->document->slotSetCurrentPage( d->pages[ d->vectorIndex - 1 ]->pageNumber() );
    }
    else
        QScrollView::wheelEvent( e );
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

    QValueVector< PageViewItem * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
    for ( ; pIt != pEnd; ++pIt )
    {
        // check if a piece of the page intersects the contents rect
        if ( (*pIt)->geometry().intersects( checkRect ) )
        {
            PageViewItem * item = *pIt;
            QRect pixmapGeometry = item->geometry();

            // translate the painter so we draw top-left pixmap corner in 0,0
            p->save();
            p->translate( pixmapGeometry.left(), pixmapGeometry.top() );

            // item pixmap and outline geometry
            QRect outlineGeometry = pixmapGeometry;
            outlineGeometry.addCoords( -1, -1, 3, 3 );

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

                // accessibility setting (TODO ADD COMPOSTING for PAGE RECOLORING)
                if ( Settings::renderMode() == Settings::EnumRenderMode::Inverted )
                    p->setRasterOp( Qt::NotCopyROP );

                // draw the pixmap
                item->page()->drawPixmap( PAGEVIEW_ID, p, pixmapRect, pixmapGeometry.width(), pixmapGeometry.height() );
            }

            p->restore();
        }
    }
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
    QValueVector< PageViewItem * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
    for ( ; pIt != pEnd; ++pIt )
    {
        PageViewItem * i = *pIt;
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

void PageView::updateZoom( ZoomMode newZoomMode )
{
    // handle zoomRect case, showing message window too
    if ( newZoomMode == ZoomRect )
    {
        d->zoomMode = ZoomRect;
        d->aZoomFitWidth->setChecked( false );
        d->aZoomFitPage->setChecked( false );
        d->aZoomFitText->setChecked( false );
        if ( !Settings::hideOSD() )
            d->messageWindow->display( i18n( "Select Zooming Area. Right-Click to zoom out." ), PageViewMessage::Info );
        return;
    }
    // if zoomMode is changing from ZoomRect, hide info popup
    if ( d->zoomMode == ZoomRect )
        d->messageWindow->hide();

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
        default:{ //ZoomFixed case
            QString z = d->aZoom->currentText();
            newFactor = KGlobal::locale()->readNumber( z.remove( z.find( '%' ), 1 ) ) / 100.0;
            }break;
        case ZoomIn:
            newFactor += (newFactor > 1.0) ? 0.2 : 0.1;
            newZoomMode = ZoomFixed;
            break;
        case ZoomOut:
            newFactor -= (newFactor > 1.0) ? 0.2 : 0.1;
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
        slotRelayoutPages();
        // request pixmaps
        slotRequestVisiblePixmaps();
        // update zoom text
        updateZoomText();
        // update actions checked state
        d->aZoomFitWidth->setChecked( checkedZoomAction == d->aZoomFitWidth );
        d->aZoomFitPage->setChecked( checkedZoomAction == d->aZoomFitPage );
        d->aZoomFitText->setChecked( checkedZoomAction == d->aZoomFitText );
        d->aZoomFitRect->setChecked( false );
    }
}

void PageView::updateZoomText()
{
    // use current page zoom as zoomFactor if in ZoomFit/* mode
    if ( d->zoomMode != ZoomFixed && d->pages.count() > 0 )
        d->zoomFactor = d->activeItem ? d->activeItem->zoomFactor() : d->pages[0]->zoomFactor();
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

//BEGIN private SLOTS
void PageView::slotRelayoutPages()
// called by: pageSetup, viewportResizeEvent, slotTwoPagesToggled, slotContinousToggled, updateZoom
{
    // set an empty container if we have no pages
    int pageCount = d->pages.count();
    if ( pageCount < 1 )
    {
        resizeContents( 0, 0 );
        return;
    }

    int viewportWidth = visibleWidth(),
        viewportHeight = visibleHeight(),
        fullWidth = 0,
        fullHeight = 0;

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
        QValueVector< PageViewItem * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
        for ( ; pIt != pEnd; ++pIt )
        {
            PageViewItem * item = *pIt;
            // update internal page size (leaving a little margin in case of Fit* modes)
            updateItemSize( item, colWidth[ cIdx ] - 10, viewportHeight - 10 );
            // find row's maximum height and column's max width
            if ( item->width() > colWidth[ cIdx ] )
                colWidth[ cIdx ] = item->width();
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
            insertY = 10; //TODO take d->zoomFactor into account (2+4*x)
        cIdx = 0;
        rIdx = 0;
        for ( pIt = d->pages.begin(); pIt != pEnd; ++pIt )
        {
            PageViewItem * item = *pIt;
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
                insertY += rHeight + 15; //(int)(5.0 + 15.0 * d->zoomFactor);
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
        PageViewItem * currentItem = d->activeItem ? d->activeItem : d->pages[0];

        // setup varialbles for a 1(row) x N(columns) grid
        int nCols = Settings::viewColumns(),
            * colWidth = new int[ nCols ],
            cIdx = 0;
        fullHeight = viewportHeight;
        for ( int i = 0; i < nCols; i++ )
            colWidth[ i ] = viewportWidth / nCols;

        // 1) find out maximum area extension for the pages
        QValueVector< PageViewItem * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
        for ( ; pIt != pEnd; ++pIt )
        {
            PageViewItem * item = *pIt;
            if ( item == currentItem || (cIdx > 0 && cIdx < nCols) )
            {
                // update internal page size (leaving a little margin in case of Fit* modes)
                updateItemSize( item, colWidth[ cIdx ] - 10, viewportHeight - 10 );
                // find row's maximum height and column's max width
                if ( item->width() > colWidth[ cIdx ] )
                    colWidth[ cIdx ] = item->width();
                if ( item->height() > fullHeight )
                    fullHeight = item->height();
                cIdx++;
            }
        }

        // 2) hide all widgets except the displayable ones and dispose those
        int insertX = 0;
        cIdx = 0;
        for ( pIt = d->pages.begin(); pIt != pEnd; ++pIt )
        {
            PageViewItem * item = *pIt;
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

    // 3) update scrollview's contents size and recenter view
    int oldWidth = contentsWidth(),
        oldHeight = contentsHeight();
    if ( oldWidth != fullWidth || oldHeight != fullHeight )
    {
        resizeContents( fullWidth, fullHeight );
        if ( oldWidth > 0 && oldHeight > 0 )
            center( fullWidth * (contentsX() + visibleWidth() / 2) / oldWidth,
                    fullHeight * (contentsY() + visibleHeight() / 2) / oldHeight );
        else
            center( fullWidth / 2, 0 );
    }

    // 4) update the viewport since a relayout is a big operation
    updateContents();

    // reset dirty state
    d->dirtyLayout = false;
}

void PageView::slotRequestVisiblePixmaps( int newLeft, int newTop )
{
    // precalc view limits for intersecting with page coords inside the lOOp
    QRect viewportRect( newLeft == -1 ? contentsX() : newLeft,
                        newTop == -1 ? contentsY() : newTop,
                        visibleWidth(), visibleHeight() );

    // scroll from the top to the last visible thumbnail
    QValueVector< PageViewItem * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
    for ( ; pIt != pEnd; ++pIt )
    {
        PageViewItem * item = *pIt;
        const QRect & itemRect = item->geometry();
        if ( viewportRect.intersects( itemRect ) )
        {
            d->document->requestPixmap( PAGEVIEW_ID, item->pageNumber(),
                                        itemRect.width(), itemRect.height(), true );
        }
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

void PageView::slotFitToRectToggled( bool on )
{
    if ( on ) updateZoom( ZoomRect );
    else updateZoom( ZoomFixed );
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
}

void PageView::slotSetMouseSelect()
{
    d->mouseMode = MouseSelection;
}

void PageView::slotSetMouseDraw()
{
    d->mouseMode = MouseEdit;
    d->aMouseEdit->setChecked( true );
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
