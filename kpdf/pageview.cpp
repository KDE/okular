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
#include <kconfigbase.h>

#include <math.h>
#include <stdlib.h>

#include "pageview.h"
#include "pageviewutils.h"
#include "pixmapwidget.h"
#include "page.h"


// structure used internally by PageView for data storage
class PageViewPrivate
{
public:
    // the document, current page and pages indices vector
    KPDFDocument * document;
    PageWidget * page; //equal to pages[vectorIndex]
    QValueVector< PageWidget * > pages;
    int vectorIndex;

    // view layout, zoom and mouse
    int viewColumns;
    bool viewContinous;
    PageView::ZoomMode zoomMode;
    float zoomFactor;
    PageView::MouseMode mouseMode;
    QPoint mouseGrabPos;
    QPoint mouseStartPos;
    bool mouseOnLink;
    bool mouseOnActiveRect;
    PageWidget * mouseSelectionWidget;

    // other stuff
    QTimer *delayTimer;
    QTimer *scrollTimer;
    int scrollIncrement;
    bool dirtyLayout;
    PageViewOverlay * overlayWindow;    //in pageviewutils.h
    PageViewMessage * messageWindow;    //in pageviewutils.h

    // actions
    KSelectAction *aZoom;
    KToggleAction *aZoomFitWidth;
    KToggleAction *aZoomFitPage;
    KToggleAction *aZoomFitText;
    KToggleAction *aZoomFitRect;
    KToggleAction *aViewTwoPages;
    KToggleAction *aViewContinous;
};



/* PageView. What's in this file? -> quick overview.
 * Code weight (in rows) and meaning:
 *  160 - constructor and creating actions plus their connected slots (empty stuff)
 *  70  - DocumentObserver inherited methodes (important)
 *  200 - events: mouse, keyboard, drag/drop
 *  170 - slotRelayoutPages: set contents of the scrollview on continous/single modes
 *  100 - zoom: zooming pages in different ways, keeping update the toolbar actions, etc..
 *  other misc functions: only slotRequestVisiblePixmaps and pickPageOnPoint noticeable,
 * and many insignificant stuff like this comment :-)
 */
PageView::PageView( QWidget *parent, KPDFDocument *document )
    : QScrollView( parent, "KPDF::pageView", WNoAutoErase | WStaticContents )
{
    // create and initialize private storage structure
    d = new PageViewPrivate();
    d->document = document;
    d->page = 0;
    d->vectorIndex = -1;
    d->viewColumns = 1;
    d->viewContinous = false;
    d->zoomMode = ZoomFixed;
    d->zoomFactor = 1.0;
    d->mouseMode = MouseNormal;
    d->mouseOnLink = false;
    d->mouseOnActiveRect = false;
    d->mouseSelectionWidget = 0;
    d->delayTimer = 0;
    d->scrollTimer = 0;
    d->scrollIncrement = 0;
    d->dirtyLayout = false;
    d->overlayWindow = 0;
    d->messageWindow = new PageViewMessage(this);

    // dealing with (very) large areas so enable clipper
    enableClipper( true );

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
    QPushButton * resizeButton = new QPushButton( viewport() );
    resizeButton->setPixmap( SmallIcon("crop") );
    setCornerWidget( resizeButton );
    resizeButton->setEnabled( false );
    // connect(...);
}

PageView::~PageView()
{
    delete d;
}

void PageView::setupActions( KActionCollection * ac, KConfigGroup * config )
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

    d->aZoomFitRect = new KToggleAction( i18n("Zoom to rect"), "viewmag", 0, ac, "zoom_fit_rect" );
    connect( d->aZoomFitRect, SIGNAL( toggled( bool ) ), SLOT( slotFitToRectToggled( bool ) ) );

    // View-Layout actions
    d->aViewTwoPages = new KToggleAction( i18n("Two Pages"), "view_left_right", 0, ac, "view_twopages" );
    connect( d->aViewTwoPages, SIGNAL( toggled( bool ) ), SLOT( slotTwoPagesToggled( bool ) ) );
    d->aViewTwoPages->setChecked( config->readBoolEntry( "ViewTwoPages", false ) );
    slotTwoPagesToggled( d->aViewTwoPages->isChecked() );

    d->aViewContinous = new KToggleAction( i18n("Continous"), "view_text", 0, ac, "view_continous" );
    connect( d->aViewContinous, SIGNAL( toggled( bool ) ), SLOT( slotContinousToggled( bool ) ) );
    d->aViewContinous->setChecked( config->readBoolEntry( "ViewContinous", true ) );
    slotContinousToggled( d->aViewContinous->isChecked() );

    // Mouse-Mode actions
    KToggleAction * mn = new KRadioAction( i18n("Normal"), "mouse", 0, this, SLOT( slotSetMouseNormal() ), ac, "mouse_drag" );
    mn->setExclusiveGroup("MouseType");
    mn->setChecked( true );

    KToggleAction *ms = new KRadioAction( i18n("Select"), "frame_edit", 0, this, SLOT( slotSetMouseSelect() ), ac, "mouse_select" );
    ms->setExclusiveGroup("MouseType");
    //ms->setEnabled( false ); // implement feature before removing this line

    md = new KRadioAction( i18n("Draw"), "edit", 0, this, SLOT( slotSetMouseDraw() ), ac, "mouse_draw" );
    md->setExclusiveGroup("MouseType");
    //md->setEnabled( false ); // implement feature before removing this line

    // Other actions
    KAction * su = new KAction( i18n("Scroll Up"), 0, this, SLOT( slotScrollUp() ), ac, "view_scroll_up" );
    su->setShortcut( "Shift+Up" );

    KAction * sd = new KAction( i18n("Scroll Down"), 0, this, SLOT( slotScrollDown() ), ac, "view_scroll_down" );
    sd->setShortcut( "Shift+Down" );

    KToggleAction * ss = new KToggleAction( i18n( "Show &Scrollbars" ), 0, ac, "show_scrollbars" );
    ss->setCheckedState(i18n("Hide &Scrollbars"));
    connect( ss, SIGNAL( toggled( bool ) ), SLOT( slotToggleScrollBars( bool ) ) );

    ss->setChecked( config->readBoolEntry( "ShowScrollBars", true ) );
    slotToggleScrollBars( ss->isChecked() );
}

void PageView::saveSettings( KConfigGroup * config )
{
    config->writeEntry( "ShowScrollBars", hScrollBarMode() == AlwaysOn );
    config->writeEntry( "ViewTwoPages", d->aViewTwoPages->isChecked() );
    config->writeEntry( "ViewContinous", d->aViewContinous->isChecked() );
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
    QValueVector< PageWidget * >::iterator dIt = d->pages.begin(), dEnd = d->pages.end();
    for ( ; dIt != dEnd; ++dIt )
        delete *dIt;
    d->pages.clear();
    d->page = 0;

    // create children widgets
    QValueVector< KPDFPage * >::const_iterator setIt = pageSet.begin(), setEnd = pageSet.end();
    for ( ; setIt != setEnd; ++setIt )
    {
        PageWidget * p = new PageWidget( viewport(), *setIt );
        p->setFocusProxy( this );
        p->setMouseTracking( true );
        d->pages.push_back( p );
    }

    // invalidate layout
    d->dirtyLayout = true;
}

void PageView::pageSetCurrent( int pageNumber, const QRect & /*viewport*/ )
{
    // select next page
    d->vectorIndex = 0;
    d->page = 0;
    QValueVector< PageWidget * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
    for ( ; pIt != pEnd; ++pIt )
    {
        if ( (*pIt)->pageNumber() == pageNumber )
        {
            d->page = *pIt;
            break;
        }
        d->vectorIndex ++;
    }
    if ( !d->page )
        return;

    // relayout in "Single Pages" mode or if a relayout is pending
    if ( !d->viewContinous || d->dirtyLayout )
        slotRelayoutPages();

    // center the view to see the selected page
    // FIXME take care of viewport
    int xPos = childX( d->page ) + d->page->widthHint() / 2,
        yPos = childY( d->page );
    center( xPos, yPos + visibleHeight() / 2 - 10 );
    slotRequestVisiblePixmaps();

    // update zoom text if in a ZoomFit/* zoom mode
    if ( d->zoomMode != ZoomFixed )
        updateZoomText();
}

void PageView::notifyPixmapChanged( int pageNumber )
{
    QValueVector< PageWidget * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
    for ( ; pIt != pEnd; ++pIt )
        if ( (*pIt)->pageNumber() == pageNumber )
        {
            (*pIt)->update();
            break;
        }
}
//END KPDFDocumentObserver inherited methods

//BEGIN widget events
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
                if ( d->mouseSelectionWidget )
                    d->mouseSelectionWidget->clearSelection();
                d->mouseSelectionWidget = 0;
                PageWidget * page = pickPageOnPoint( e->x(), e->y() );
                if ( page )
                {
                    page->setBeginCorner( e->x() - childX( page ), e->y() - childY( page ) );
                    d->mouseSelectionWidget = page;
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
        float zoom = QMIN( (float)clipper()->width() / (float)selRect.width(),
                           (float)clipper()->height() / (float)selRect.height() );

        // get normalized view center (relative to the contentsRect)
        // coeffs (2.0 and 1.5) are for correcting the asymmetic page border
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

    PageWidget * pageWidget = pickPageOnPoint( e->x(), e->y() );
    switch ( d->mouseMode )
    {
        case MouseNormal:
            setCursor( arrowCursor );
            if ( leftButton && pageWidget )
            {
                if ( d->mouseOnLink )
                {
                    // activate link
                    int linkX = e->x() - childX( pageWidget ),
                        linkY = e->y() - childY( pageWidget );
                    d->document->slotProcessLink( pageWidget->page()->getLink( linkX, linkY ) );
                }
                else
                {
                    // mouse not moved since press, so we have a click. select the page.
                    if ( e->globalPos() == d->mouseStartPos )
                        d->document->slotSetCurrentPage( pageWidget->pageNumber() );
                }
            }
            else if ( rightButton && pageWidget )
            {
                // if over a page display a popup menu
                const KPDFPage * kpdfPage = pageWidget->page();
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
            if ( leftButton && d->mouseSelectionWidget )
            {
                // request the textpage if there isn't one
                const KPDFPage * kpdfPage = d->mouseSelectionWidget->page();
                if ( !kpdfPage->hasSearchPage() )
                    d->document->requestTextPage( kpdfPage->number() );

                // copy text into the clipboard
                QClipboard *cb = QApplication::clipboard();
                const QString & selection = d->mouseSelectionWidget->selectedText();
                cb->setText( selection, QClipboard::Clipboard );
                if ( cb->supportsSelection() )
                    cb->setText( selection, QClipboard::Selection );

                // clear widget selection
                d->mouseSelectionWidget->clearSelection();
                d->mouseSelectionWidget = 0;
            }
            break;

        case MouseEdit:      // ? apply [tool] ?
            break;
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
            d->overlayWindow = new PageViewOverlay( clipper(), PageViewOverlay::Zoom );
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
                // detect the underlaying page (if present)
                PageWidget * pageWidget = pickPageOnPoint( e->x(), e->y() );
                if ( pageWidget )
                {
                    int pageX = e->x() - childX( pageWidget ),
                        pageY = e->y() - childY( pageWidget );

                    // check if over a KPDFActiveRect
                    bool onActiveRect = pageWidget->page()->hasActiveRect( pageX, pageY );
                    if ( onActiveRect != d->mouseOnActiveRect )
                    {
                        d->mouseOnActiveRect = onActiveRect;
                        setCursor( onActiveRect ? pointingHandCursor : arrowCursor );
                    }

                    // check if over a KPDFLink
                    bool onLink = pageWidget->page()->hasLink( pageX, pageY );
                    if ( onLink != d->mouseOnLink )
                    {
                        d->mouseOnLink = onLink;
                        setCursor( onLink ? pointingHandCursor : arrowCursor );
                    }
                }
            }
            break;

        case MouseSelection:
            // set second corner of selection in selection pagewidget
            if ( leftButton && d->mouseSelectionWidget )
                d->mouseSelectionWidget->setEndCorner( e->x() - childX( d->mouseSelectionWidget ),
                                                       e->y() - childY( d->mouseSelectionWidget ) );
            break;

        case MouseEdit:      // ? update graphics ?
            break;
    }
}

void PageView::keyPressEvent( QKeyEvent * e )
{
    e->accept();
    // move/scroll page by using keys
    switch ( e->key() )
    {
        case Key_Up:
            if ( atTop() && !d->viewContinous )
                scrollUp();
            else
                verticalScrollBar()->subtractLine();
            break;
        case Key_Down:
            if ( atBottom() && !d->viewContinous )
                scrollDown();
            else
                verticalScrollBar()->addLine();
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

void PageView::wheelEvent( QWheelEvent *e )
{
    int delta = e->delta();
    e->accept();
    if ( (e->state() & ControlButton) == ControlButton ) {
        if ( e->delta() < 0 )
            slotZoomOut();
        else
            slotZoomIn();
    }
    else if ( delta <= -120 && atBottom() && !d->viewContinous )
        scrollDown();
    else if ( delta >= 120 && atTop() && !d->viewContinous )
        scrollUp();
    else
        QScrollView::wheelEvent( e );
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

//BEGIN internal SLOTS
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
    int newColumns = on ? 2 : 1;
    if ( d->viewColumns != newColumns )
    {
        d->viewColumns = newColumns;
        slotRelayoutPages();
    }
}

void PageView::slotContinousToggled( bool on )
{
    if ( d->viewContinous != on )
    {
        d->viewContinous = on;
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
    md->setChecked( true );
}

void PageView::slotScrollUp()
{
    if ( d->scrollIncrement < -9 )
        return;
    d->scrollIncrement--;
    slotAutoScoll();
}

void PageView::slotScrollDown()
{
    if ( d->scrollIncrement > 9 )
        return;
    d->scrollIncrement++;
    slotAutoScoll();
}

void PageView::slotToggleScrollBars( bool on )
{
    setHScrollBarMode( on ? AlwaysOn : AlwaysOff );
    setVScrollBarMode( on ? AlwaysOn : AlwaysOff );
}

void PageView::slotRelayoutPages()
// called by: pageSetup, viewportResizeEvent, slotTwoPagesToggled, slotContinousToggled, updateZoom
{
    // set an empty container if we have no pages
    int pageCount = d->pages.count();
    if ( pageCount < 1 )
    {
        resizeContents( 0,0 );
        return;
    }

    int viewportWidth = clipper()->width(),
        viewportHeight = clipper()->height(),
        fullWidth = 0,
        fullHeight = 0;

    if ( d->viewContinous == TRUE )
    {
        // Here we find out column's width and row's height to compute a table
        // so we can place widgets 'centered in virtual cells'.
        int nCols = d->viewColumns,
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
        QValueVector< PageWidget * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
        for ( ; pIt != pEnd; ++pIt )
        {
            PageWidget * p = *pIt;
            // update internal page geometry
            if ( d->zoomMode == ZoomFixed )
                p->setZoomFixed( d->zoomFactor );
            else if ( d->zoomMode == ZoomFitWidth )
                p->setZoomFitWidth( colWidth[ cIdx ] - 10 );
            else
                p->setZoomFitRect( colWidth[ cIdx ] - 10, viewportHeight - 10 );
            // find row's maximum height and column's max width
            int pWidth = p->widthHint(),
                pHeight = p->heightHint();
            if ( pWidth > colWidth[ cIdx ] )
                colWidth[ cIdx ] = pWidth;
            if ( pHeight > rowHeight[ rIdx ] )
                rowHeight[ rIdx ] = pHeight;
            // update col/row indices
            if ( ++cIdx == nCols )
            {
                cIdx = 0;
                rIdx++;
            }
        }

        // 2) arrange widgets inside cells
        int insertX = 0,
            insertY = (int)(2.0 + 4.0 * d->zoomFactor);
        cIdx = 0;
        rIdx = 0;
        for ( pIt = d->pages.begin(); pIt != pEnd; ++pIt )
        {
            PageWidget * p = *pIt;
            int pWidth = p->widthHint(),
                pHeight = p->heightHint(),
                cWidth = colWidth[ cIdx ],
                rHeight = rowHeight[ rIdx ];
            // show, resize and center widget inside 'cells'
            p->resize( pWidth, pHeight );
            moveChild( p, insertX + (cWidth - pWidth) / 2,
                       insertY + (rHeight - pHeight) / 2 );
            p->show();
            // advance col/row index
            insertX += cWidth;
            if ( ++cIdx == nCols )
            {
                cIdx = 0;
                rIdx++;
                insertX = 0;
                insertY += rHeight + (int)(5.0 + 15.0 * d->zoomFactor);
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
        PageWidget * currentPage = d->page ? d->page : d->pages[0];

        // setup varialbles for a 1(row) x N(columns) grid
        int nCols = d->viewColumns,
            * colWidth = new int[ nCols ],
            cIdx = 0;
        fullHeight = viewportHeight;
        for ( int i = 0; i < nCols; i++ )
            colWidth[ i ] = viewportWidth / nCols;

        // 1) find out maximum area extension for the pages
        QValueVector< PageWidget * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
        for ( ; pIt != pEnd; ++pIt )
        {
            PageWidget * p = *pIt;
            if ( p == currentPage || (cIdx > 0 && cIdx < nCols) )
            {
                if ( d->zoomMode == ZoomFixed )
                    p->setZoomFixed( d->zoomFactor );
                else if ( d->zoomMode == ZoomFitWidth )
                    p->setZoomFitWidth( colWidth[ cIdx ] - 10 );
                else
                    p->setZoomFitRect( colWidth[ cIdx ] - 10, viewportHeight - 10 );
                if ( p->widthHint() > colWidth[ cIdx ] )
                    colWidth[ cIdx ] = p->widthHint();
                fullHeight = QMAX( fullHeight, p->heightHint() );
                cIdx++;
            }
        }

        // 2) hide all widgets except the displayable ones and dispose those
        int insertX = 0,
            insertY = (int)(2.0 + 4.0 * d->zoomFactor);
        cIdx = 0;
        for ( pIt = d->pages.begin(); pIt != pEnd; ++pIt )
        {
            PageWidget * p = *pIt;
            if ( p == currentPage || (cIdx > 0 && cIdx < nCols) )
            {
                int pWidth = p->widthHint(),
                    pHeight = p->heightHint();
                // show, resize and center widget inside 'cells'
                p->resize( pWidth, pHeight );
                moveChild( p, insertX + (colWidth[ cIdx ] - pWidth) / 2,
                           insertY + (fullHeight - pHeight) / 2 );
                p->show();
                // advance col/row index
                insertX += colWidth[ cIdx ];
                cIdx++;
            }
            else
                p->hide();
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

    // reset dirty state
    d->dirtyLayout = false;
}

void PageView::slotRequestVisiblePixmaps( int newLeft, int newTop )
{
    // precalc view limits for intersecting with page coords inside the lOOp
    int vLeft = (newLeft == -1) ? contentsX() : newLeft,
        vRight = vLeft + visibleWidth(),
        vTop = (newTop == -1) ? contentsY() : newTop,
        vBottom = vTop + visibleHeight();

    // scroll from the top to the last visible thumbnail
    QValueVector< PageWidget * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
    for ( ; pIt != pEnd; ++pIt )
    {
        PageWidget * p = *pIt;
        int pLeft = childX( p ),
            pRight = pLeft + p->widthHint(),
            pTop = childY( p ),
            pBottom = pTop + p->heightHint();
        if ( p->isShown() && pRight > vLeft && pLeft < vRight && pBottom > vTop && pTop < vBottom )
            d->document->requestPixmap( PAGEVIEW_ID, p->pageNumber(), p->pixmapWidth(), p->pixmapHeight(), true );
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
//END internal SLOTS

void PageView::updateZoom( ZoomMode newZoomMode )
{
    // handle zoomRect case, showing message window too
    if ( newZoomMode == ZoomRect )
    {
        d->zoomMode = ZoomRect;
        d->aZoomFitWidth->setChecked( false );
        d->aZoomFitPage->setChecked( false );
        d->aZoomFitText->setChecked( false );
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
            if ( newFactor < 0.1 || newFactor > 8.0 )
                return;
            }break;
        case ZoomIn:
            newFactor += 0.1;
            if ( newFactor >= 4.0 )
                newFactor = 4.0;
            newZoomMode = ZoomFixed;
            break;
        case ZoomOut:
            newFactor -= 0.1;
            if ( newFactor <= 0.125 )
                newFactor = 0.125;
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

    if ( newZoomMode != d->zoomMode || (newZoomMode == ZoomFixed && newFactor != d->zoomFactor ) )
    {
        // rebuild layout and change the zoom selectAction contents
        d->zoomMode = newZoomMode;
        d->zoomFactor = newFactor;
        slotRelayoutPages();
        updateZoomText();
        // update actions checked state
        d->aZoomFitWidth->setChecked( checkedZoomAction == d->aZoomFitWidth );
        d->aZoomFitPage->setChecked( checkedZoomAction == d->aZoomFitPage );
        d->aZoomFitText->setChecked( checkedZoomAction == d->aZoomFitText );
        d->aZoomFitRect->setChecked( false );
        // request pixmaps
        slotRequestVisiblePixmaps();
    }
}

void PageView::updateZoomText()
{
    // use current page zoom as zoomFactor if in ZoomFit/* mode
    if ( d->zoomMode != ZoomFixed && d->pages.count() > 0 )
        d->zoomFactor = d->page ? d->page->zoomFactor() : d->pages[0]->zoomFactor();
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

PageWidget * PageView::pickPageOnPoint( int x, int y )
{
    PageWidget * page = 0;
    QValueVector< PageWidget * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
    for ( ; pIt != pEnd; ++pIt )
    {
        PageWidget * p = *pIt;
        int pLeft = childX( p ),
            pRight = pLeft + p->widthHint(),
            pTop = childY( p ),
            pBottom = pTop + p->heightHint();
        // little optimized, stops if found or probably quits on the next row
        if ( x > pLeft && x < pRight && y < pBottom && p->isShown() )
        {
            if ( y > pTop )
                page = p;
            break;
        }
    }
    return page;
}

bool PageView::atTop() const
{
    return verticalScrollBar()->value() == verticalScrollBar()->minValue();
}

bool PageView::atBottom() const
{
    return verticalScrollBar()->value() == verticalScrollBar()->maxValue();
}

void PageView::scrollUp()
{
    if( atTop() && d->vectorIndex > 0 )
        // go to the bottom of previous page
        d->document->slotSetCurrentPage( d->pages[ d->vectorIndex - 1 ]->pageNumber() ); //TODO add position
    else
    {   // go towards the top of current page
        int newValue = QMAX( verticalScrollBar()->value() - height() + 50,
                             verticalScrollBar()->minValue() );
        verticalScrollBar()->setValue( newValue );
    }
}

void PageView::scrollDown()
{
    if( atBottom() && d->vectorIndex < (int)d->pages.count() - 1 )
        // go to the top of previous page
        d->document->slotSetCurrentPage( d->pages[ d->vectorIndex + 1 ]->pageNumber() ); // TODO add position
    else
    {    // go towards the bottom of current page
        int newValue = QMIN( verticalScrollBar()->value() + height() - 50,
                             verticalScrollBar()->maxValue() );
        verticalScrollBar()->setValue( newValue );
    }
}

#include "pageview.moc"
