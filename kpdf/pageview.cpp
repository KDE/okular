/***************************************************************************
 *   Copyright (C) 2002 by Wilco Greven <greven@kde.org>                   *
 *   Copyright (C) 2003 by Christophe Devriese                             *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Laurent Montel <montel@kde.org>                 *
 *   Copyright (C) 2003 by Dirk Mueller <mueller@kde.org>                  *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *   Copyright (C) 2004 by James Ots <kde@jamesots.com>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qcursor.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qpushbutton.h>

#include <kglobalsettings.h> 
#include <kiconloader.h>
#include <kurldrag.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kconfigbase.h>

#include <math.h>

#include "pageview.h"
#include "pixmapwidget.h"
#include "page.h"


// structure used internally by PageView for data storage
class PageViewPrivate
{
public:
    // the document, current page and pages indices vector
    KPDFDocument * document;
    const KPDFPage * page; //equal to pages[vectorIndex]
    QValueVector< PageWidget * > pages;
    int vectorIndex;

    // view layout, zoom and mouse
    PageView::ViewMode viewMode;
    PageView::ZoomMode zoomMode;
    float zoomFactor;
    PageView::MouseMode mouseMode;
    QPoint mouseGrabPos;
    bool mouseOnLink;
    QTimer *delayTimer;

    // actions
    KSelectAction *aZoom;
    KToggleAction *aZoomFitWidth;
    KToggleAction *aZoomFitPage;
    KToggleAction *aZoomFitText;

    //TODO ADD  bool dirty;
};



/*
 * PageView class
 */
PageView::PageView( QWidget *parent, KPDFDocument *document )
    : QScrollView( parent, "KPDF::pageView", WRepaintNoErase | WStaticContents )
{
    // create and initialize private storage structure
    d = new PageViewPrivate();
    d->document = document;
    d->page = 0;
    d->mouseMode = MouseNormal;
    d->mouseOnLink = false;
    d->zoomMode = ZoomFixed;
    d->zoomFactor = 0.1; //FIXME REDEFAULT!!
    d->delayTimer = 0;

    // widget setup
    setAcceptDrops( true );
    setFocusPolicy( QWidget::StrongFocus );
    viewport()->setFocusPolicy( QWidget::WheelFocus );
    viewport()->setMouseTracking( true );
    enableClipper( true );

    // set a corner button to resize the view to the page size
    QPushButton * resizeButton = new QPushButton( viewport() );
    resizeButton->setPixmap( SmallIcon("crop" /*"top"*/) );
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
    // Zoom actions ( higher scales consumes lots of memory! )
    const double zoomValue[10] = { 0.125, 0.25, 0.333, 0.5, 0.667, 0.75, 1, 1.25, 1.50, 2 };

    d->aZoom = new KSelectAction( i18n( "Zoom" ), "viewmag", 0, ac, "zoom_to" );
    connect( d->aZoom, SIGNAL( activated( const QString & ) ), this, SLOT( slotZoom( const QString& ) ) );
    d->aZoom->setEditable(  true );

    QStringList translated;
    translated << i18n("Fit Width") << i18n("Fit Page");
    QString localValue;
    QString double_oh( "00" );
    for ( int i = 0; i < 10; i++ )
    {
        localValue = KGlobal::locale()->formatNumber( zoomValue[i] * 100.0, 2 );
        localValue.remove( KGlobal::locale()->decimalSymbol() + double_oh );
        translated << QString( "%1%" ).arg( localValue );
    }
    d->aZoom->setItems( translated );
    d->aZoom->setCurrentItem( 8 );

    KStdAction::zoomIn( this, SLOT( slotZoomIn() ), ac, "zoom_in" );

    KStdAction::zoomOut( this, SLOT( slotZoomOut() ), ac, "zoom_out" );

    d->aZoomFitWidth = new KToggleAction( i18n("Fit to Page &Width"), "viewmagfit", 0, ac, "zoom_fit_width" );
    connect( d->aZoomFitWidth, SIGNAL( toggled( bool ) ), SLOT( slotFitToWidthToggled( bool ) ) );

    d->aZoomFitPage = new KToggleAction( i18n("Fit to &Page"), "viewmagfit", 0, ac, "zoom_fit_page" );
    connect( d->aZoomFitPage, SIGNAL( toggled( bool ) ), SLOT( slotFitToPageToggled( bool ) ) );

    // View-Layout actions
    KToggleAction * vs = new KToggleAction( i18n("Single Page"), "view_remove", 0, this, SLOT( slotSetViewSingle() ), ac, "view_single" );
    vs->setExclusiveGroup("ViewLayout");
    vs->setChecked( true );

    KToggleAction * vd = new KToggleAction( i18n("Two Pages"), "view_left_right", 0, this, SLOT( slotSetViewDouble() ), ac, "view_double" );
    vd->setExclusiveGroup("ViewLayout");
    vd->setEnabled( false ); // implement feature before removing this line

    KToggleAction * vc = new KToggleAction( i18n("Continous"), "view_text", 0, this, SLOT( slotSetViewContinous() ), ac, "view_continous" );
    vc->setExclusiveGroup("ViewLayout"); //TODO toggable with others or.. alone?
    vc->setEnabled( false ); // implement feature before removing this line

    // Mouse-Mode actions
    KToggleAction * mn = new KToggleAction( i18n("Normal"), "mouse", 0, this, SLOT( slotSetMouseNormal() ), ac, "mouse_drag" );
    mn->setExclusiveGroup("MouseType");
    mn->setChecked( true );

    KToggleAction * ms = new KToggleAction( i18n("Select"), "frame_edit", 0, this, SLOT( slotSetMouseSelect() ), ac, "mouse_select" );
    ms->setExclusiveGroup("MouseType");
    ms->setEnabled( false ); // implement feature before removing this line

    KToggleAction * md = new KToggleAction( i18n("Draw"), "edit", 0, this, SLOT( slotSetMouseDraw() ), ac, "mouse_draw" );
    md->setExclusiveGroup("MouseType");
    md->setEnabled( false ); // implement feature before removing this line

    // Other actions
    KToggleAction * ss = new KToggleAction( i18n( "Show &Scrollbars" ), 0, ac, "show_scrollbars" );
    ss->setCheckedState(i18n("Hide &Scrollbars"));
    connect( ss, SIGNAL( toggled( bool ) ), SLOT( slotToggleScrollBars( bool ) ) );

    ss->setChecked( config->readBoolEntry( "ShowScrollBars", true ) );
    slotToggleScrollBars( ss->isChecked() );
}

void PageView::saveSettings( KConfigGroup * config )
{
    config->writeEntry( "ShowScrollBars", hScrollBarMode() == AlwaysOn );
}


//BEGIN KPDFDocumentObserver inherited methods 
void PageView::pageSetup( const QValueVector<KPDFPage*> & pageSet, bool /*documentChanged*/ )
{ /* TODO: preserve (reuse) existing pages if !documentChanged */
    // delete all pages
    QValueVector< PageWidget * >::iterator dIt = d->pages.begin(), dEnd = d->pages.end();
    for ( ; dIt != dEnd; ++dIt )
        delete *dIt;
    d->pages.clear();
    d->page = 0;

    // create children widgets
    QValueVector< KPDFPage * >::const_iterator setIt = pageSet.begin(), setEnd = pageSet.end();
    for ( ; setIt != setEnd; ++setIt )
    {
        PageWidget * p = new PageWidget( this, *setIt );
        // add to the internal queue
        d->pages.push_back( p );
        // add to the scrollview
        addChild( p, 0, 0 );
    }

    // FIXME TEMP relayout
    reLayoutPages();
}

#define FIXME_COLS 3
void PageView::reLayoutPages()
{
    int pageCount = d->pages.count();
    if ( pageCount < 1 )
    {
        resizeContents( 0,0 );
        return;
    }

    // 1) preprocess array (compute 'virtual grid' values)
    // Here we find out column's width and row's height to compute a table
    // so we can place widgets 'centered in virtual cells'.
    int nCols = FIXME_COLS,
        nRows = (int)ceilf( (float)pageCount / (float)nCols ),
        cIdx = 0,
        rIdx = 0,
        vWidth = clipper()->width() / nCols,
        vHeight = clipper()->height();
    int * cSize = new int[ nCols ],
        * rSize = new int[ nRows ];

    for ( int i = 0; i < nCols; i++ )
        cSize[ i ] = vWidth;
    for ( int i = 0; i < nRows; i++ )
        rSize[ i ] = 0;

    QValueVector< PageWidget * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
    for ( ; pIt != pEnd; ++pIt )
    {
        PageWidget * p = *pIt;
        // update internal page geometry
        switch ( d->zoomMode )
        {
        case ZoomFitWidth:
            p->setZoomFitWidth( vWidth - 5 );
            break;
        case ZoomFitPage:
            p->setZoomFitRect( vWidth - 5, vHeight - 5 );
            break;
        default:
            p->setZoomFixed( d->zoomFactor );
        }
        // find maximum for 'col' / 'row'
        int pWidth = p->widthHint(),
            pHeight = p->heightHint();
        if ( pWidth > cSize[ cIdx ] )
            cSize[ cIdx ] = pWidth;
        if ( pHeight > rSize[ rIdx ] )
            rSize[ rIdx ] = pHeight;
        // update col/row indices
        if ( ++cIdx == nCols )
        {
            cIdx = 0;
            rIdx++;
        }
    }

    // 2) dispose widgets inside cells
    int currentHeight = 0,
        currentWidth = 0;
    cIdx = 0;
    rIdx = 0;
    for ( pIt = d->pages.begin(); pIt != pEnd; ++pIt )
    {
        PageWidget * p = *pIt;
        int pWidth = p->widthHint(),
            pHeight = p->heightHint(),
            cWidth = cSize[ cIdx ],
            rHeight = rSize[ rIdx ];
        // center widget inside 'cells'
        moveChild( p, currentWidth + (cWidth - pWidth) / 2,
                      currentHeight + (rHeight - pHeight) / 2 );
        // display the widget
        p->show();
        // advance col/row index
        currentWidth += cWidth;
        if ( ++cIdx == nCols )
        {
            cIdx = 0;
            rIdx++;
            currentWidth = 0;
            currentHeight += rHeight + 10;
        }
    }

    // update scrollview's contents size (sets scrollbars limits)
    currentWidth = 0;
    for ( int i = 0; i < nCols; i++ )
        currentWidth += cSize[ i ];
    if ( cIdx != 0 )
        currentHeight += rSize[ rIdx ] + 10;
    resizeContents( currentWidth, currentHeight );

    delete [] cSize;
    delete [] rSize;

/*
    // request for thumbnail generation
    // an update is already scheduled, so don't proceed
    if ( m_delayTimer && m_delayTimer->isActive() )
        return;

    int vHeight = visibleHeight(),
        vOffset = newContentsY == -1 ? contentsY() : newContentsY;

    // scroll from the top to the last visible thumbnail
    QValueVector<ThumbnailWidget *>::iterator thumbIt = m_thumbnails.begin();
    QValueVector<ThumbnailWidget *>::iterator thumbEnd = m_thumbnails.end();
    for ( ; thumbIt != thumbEnd; ++thumbIt )
    {
        ThumbnailWidget * t = *thumbIt;
        int top = childY( t ) - vOffset;
        if ( top > vHeight )
            break;
        else if ( top + t->height() > 0 )
            m_document->requestPixmap( THUMBNAILS_ID, t->pageNumber(), t->pixmapWidth(), t->pixmapHeight(), true );
    }
*/
}

void PageView::pageSetCurrent( int /*pageNumber*/, float /*position*/ )
{
    // select next page
    d->vectorIndex = 0;
    d->page = 0;

    /*
    QValueVector<int>::iterator pagesIt = d->pages.begin();
    QValueVector<int>::iterator pagesEnd = d->pages.end();
    for ( ; pagesIt != pagesEnd; ++pagesIt )
    {
        if ( *pagesIt == pageNumber )
        {
            d->page = d->document->page( pageNumber );
            break;
        }
        d->vectorIndex++;
    }
    */
    //if ( !d->page || d->page->width() < 1 || d->page->height() < 1 )
    //    return;

    //slotUpdateView();
    //verticalScrollBar()->setValue( (int)(position * verticalScrollBar()->maxValue()) );
}

void PageView::notifyPixmapChanged( int pageNumber )
{
    // check if it's the preview we're waiting for and update it
    if ( d->page && (int)d->page->number() == pageNumber )
        slotUpdateView();
}
//END KPDFDocumentObserver inherited methods

//BEGIN widget events 
void PageView::contentsMousePressEvent( QMouseEvent * e )
{
    switch ( d->mouseMode )
    {
    case MouseNormal:    // drag / click start
        if ( e->button() & LeftButton )
        {
            d->mouseGrabPos = e->globalPos();
            setCursor( sizeAllCursor );
        }
        else if ( e->button() & RightButton )
            emit rightClick();

        /* TODO Albert
            note: 'Page' is an 'information container' and has to deal with clean
            data (such as the '(int)page & (float)position' where link refers to,
            not a LinkAction struct.. better an own struct). 'Document' is the place
            to put xpdf/Splash dependant stuff and fill up pages with interpreted
            data. I think is a *clean* way to handle everything.
            d->pressedLink = *PAGE* or *DOCUMENT* ->findLink( normalizedX, normY );
        */
        break;

    case MouseSelection: // ? set 1st corner of the selection rect ?

    case MouseEdit:      // ? place the beginning of [tool] ?
        break;
    }
}

void PageView::contentsMouseReleaseEvent( QMouseEvent * )
{
    switch ( d->mouseMode )
    {
    case MouseNormal:    // end drag / follow link
        setCursor( arrowCursor );

        /* TODO Albert
            PageLink * link = *PAGE* ->findLink(e->x()/m_ppp, e->y()/m_ppp);
            if ( link == d->pressedLink )
                //go to link, use:
                document->slotSetCurrentPagePosition( (int)link->page(), (float)link->position() );
                //and all the views will update and display the right page at the right position
            d->pressedLink = 0;
        */
        break;

    case MouseSelection: // ? d->page->setPixmapOverlaySelection( QRect ) ?

    case MouseEdit:      // ? apply [tool] ?
        break;
    }
}

void PageView::contentsMouseMoveEvent( QMouseEvent * e )
{
    switch ( d->mouseMode )
    {
    case MouseNormal:    // move page / change mouse cursor if over links
        if ( e->state() & LeftButton )
        {
            QPoint delta = d->mouseGrabPos - e->globalPos();
            scrollBy( delta.x(), delta.y() );
            d->mouseGrabPos = e->globalPos();
        }
        /* TODO Albert
            LinkAction* action = *PAGE* ->findLink(e->x()/m_ppp, e->y()/m_ppp);
            setCursor(action != 0 ? );
            experimental version using Page->hasLink( int pageX, int pageY )
            and haslink has a fake true response on
        */
        // EROS FIXME find right page for query
        /*if ( d->page && e->state() == NoButton &&  )
        {
            bool onLink = d->page->hasLink( e->x() - d->pageRect.left(), e->y() - d->pageRect.top() );
            // set cursor only when entering / leaving (setCursor has not an internal cache)
            if ( onLink != d->mouseOnLink )
            {
                d->mouseOnLink = onLink;
                setCursor( onLink ? pointingHandCursor : arrowCursor );
            }
        }
        */
        break;

    case MouseSelection: // ? update selection contour ?

    case MouseEdit:      // ? update graphics ?
        break;
    }
}

void PageView::viewportResizeEvent( QResizeEvent * )
{
    // start a timer that will refresh the pixmap after 0.5s
    if ( !d->delayTimer )
    {
        d->delayTimer = new QTimer( this );
        connect( d->delayTimer, SIGNAL( timeout() ), this, SLOT( slotUpdateView() ) );
    }
    d->delayTimer->start( 400, true );
    // recalc coordinates
    //slotUpdateView( false );
}

void PageView::keyPressEvent( QKeyEvent * e )
{
    switch ( e->key() )
    {
    case Key_Up:
        if ( atTop() )
            scrollUp();
        else
            verticalScrollBar()->subtractLine();
        break;
    case Key_Down:
        if ( atBottom() )
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
    case Key_Space:
        if( e->state() != ShiftButton )
            scrollDown();
        break;
    default:
        e->ignore();
        return;
    }
    e->accept();
}

void PageView::wheelEvent( QWheelEvent *e )
{
    int delta = e->delta();
    e->accept();
    if ( (e->state() & ControlButton) == ControlButton ) {
        if ( e->delta() > 0 )
            slotZoomOut();
        else
            slotZoomIn();
    }
    else if ( delta <= -120 && atBottom() )
        scrollDown();
    else if ( delta >= 120 && atTop() )
        scrollUp();
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

//BEGIN internal SLOTS
void PageView::slotZoom( const QString & nz )
{
    if ( nz == i18n("Fit Width") )
    {
        d->aZoomFitWidth->setChecked( true );
        return slotFitToWidthToggled( true );
    }
    if ( nz == i18n("Fit Page") )
    {
        d->aZoomFitPage->setChecked( true );
        return slotFitToPageToggled( true );
    }

    QString z = nz;
    z.remove( z.find( '%' ), 1 );
    bool isNumber = true;
    double zoom = KGlobal::locale()->readNumber(  z, &isNumber ) / 100;

    if ( d->zoomFactor != zoom && zoom > 0.1 && zoom < 8.0 )
    {
        d->zoomMode = ZoomFixed;
        d->zoomFactor = zoom;
        slotUpdateView();
        d->aZoomFitWidth->setChecked( false );
        d->aZoomFitPage->setChecked( false );
    }
}

void PageView::slotZoomIn()
{
    if ( d->zoomFactor >= 4.0 )
        return;
    d->zoomFactor += 0.1;
    if ( d->zoomFactor >= 4.0 )
        d->zoomFactor = 4.0;

    d->zoomMode = ZoomFixed;
    slotUpdateView();
    d->aZoomFitWidth->setChecked( false );
    d->aZoomFitPage->setChecked( false );
}

void PageView::slotZoomOut()
{
    if ( d->zoomFactor <= 0.125 )
        return;
    d->zoomFactor -= 0.1;
    if ( d->zoomFactor <= 0.125 )
        d->zoomFactor = 0.125;

    d->zoomMode = ZoomFixed;
    slotUpdateView();
    d->aZoomFitWidth->setChecked( false );
    d->aZoomFitPage->setChecked( false );
}

void PageView::slotFitToWidthToggled( bool on )
{
    d->zoomMode = on ? ZoomFitWidth : ZoomFixed;
    slotUpdateView();
    d->aZoomFitPage->setChecked( false );
    //FIXME uncheck others (such as FitToText)
}

void PageView::slotFitToPageToggled( bool on )
{
    d->zoomMode = on ? ZoomFitPage : ZoomFixed;
    slotUpdateView();
    d->aZoomFitWidth->setChecked( false );
}

void PageView::slotFitToTextToggled( bool on )
{
    d->zoomMode = on ? ZoomFitText : ZoomFixed;
    slotUpdateView();
    d->aZoomFitWidth->setChecked( false );
}

void PageView::slotSetViewSingle()
{    //TODO this
}

void PageView::slotSetViewDouble()
{    //TODO this
}

void PageView::slotSetViewContinous()
{    //TODO this
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
}

void PageView::slotToggleScrollBars( bool on )
{
    setHScrollBarMode( on ? AlwaysOn : AlwaysOff );
    setVScrollBarMode( on ? AlwaysOn : AlwaysOff );
}

void PageView::slotUpdateView( bool /*repaint*/ )
{    //TODO ASYNC autogeneration!
   reLayoutPages();
/*    if ( !d->page )
    {
        d->pageRect.setRect( 0, 0, 0, 0 );
        resizeContents( 0, 0 );
    }
    else
    {
        // Zoom / AutoFit-Width / AutoFit-Page
        double scale = d->zoomFactor;
        if ( d->zoomMode == ZoomFitWidth || d->zoomMode == ZoomFitPage )
        {
            scale = (double)viewport()->width() / (double)d->page->width();
            if ( d->zoomMode == ZoomFitPage )
            {
                double scaleH = (double)viewport()->height() / (double)d->page->height();
                if ( scaleH < scale )
                    scale = scaleH;
            }
        }
        int pageW = (int)( scale * d->page->width() ),
            pageH = (int)( scale * d->page->height() ),
            viewW = QMAX( viewport()->width(), pageW ),
            viewH = QMAX( viewport()->height(), pageH );
        d->pageRect.setRect( (viewW - pageW) / 2, (viewH - pageH) / 2, pageW, pageH );
        resizeContents( viewW, viewH );
        d->document->requestPixmap( PAGEVIEW_ID, d->page->number(), pageW, pageH, true );
    }
    if ( repaint )
        viewport()->update();
*/
}
//END internal SLOTS

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
        d->document->slotSetCurrentPagePosition( d->pages[ d->vectorIndex - 1 ]->pageNumber(), 1.0 );
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
        d->document->slotSetCurrentPagePosition( d->pages[ d->vectorIndex + 1 ]->pageNumber(), 0.0 );
    else
    {    // go towards the bottom of current page
        int newValue = QMIN( verticalScrollBar()->value() + height() - 50,
                             verticalScrollBar()->maxValue() );
        verticalScrollBar()->setValue( newValue );
    }
}

#include "pageview.moc"
