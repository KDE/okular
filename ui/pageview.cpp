/***************************************************************************
 *   Copyright (C) 2004-2005 by Enrico Ros <eros.kde@email.it>             *
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

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <fixx11h.h>

// qt/kde includes
#include <qcursor.h>
#include <qevent.h>
#include <qgraphicsscene.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qpushbutton.h>
#include <qset.h>
#include <qscrollbar.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <QX11Info>
#include <kiconloader.h>
#include <kaction.h>
#include <kstdaccel.h>
#include <kstdaction.h>
#include <kactioncollection.h>
#include <kmenu.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kimageeffect.h>
#include <kselectaction.h>
#include <ktoggleaction.h>
#include <ktoolinvocation.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kicon.h>
#include <QtDBus/QtDBus>

// system includes
#include <math.h>
#include <stdlib.h>

// local includes
#include "pageview.h"
#include "pageviewutils.h"
#include "pagepainter.h"
#include "pageviewannotator.h"
#include "core/document.h"
#include "core/page.h"
#include "core/misc.h"
#include "core/link.h"
#include "core/generator.h"
#include "settings.h"

#define ROUND(x) (int(x + 0.5))

// structure used internally by PageView for data storage
class PageViewPrivate
{
public:
    // the document, pageviewItems and the 'visible cache'
    KPDFDocument * document;
    QVector< PageViewItem * > items;
    QLinkedList< PageViewItem * > visibleItems;

    // view layout (columns and continuous in Settings), zoom and mouse
    PageView::ZoomMode zoomMode;
    float zoomFactor;
    PageView::MouseMode mouseMode;
    QPoint mouseGrabPos;
    QPoint mousePressPos;
    QPoint mouseSelectPos;
    bool mouseMidZooming;
    int mouseMidLastY;
    bool mouseSelecting;
    RubberBandItem * rubberBand;
    bool mouseTextSelecting;
/*
    bool mouseTextSelectionPainted;
    QList<QRect>* mouseTextSelectionRect;
    QColor mouseTextSelectionColor;
    TextSelection * mouseTextSelectionInfo;
*/
    QSet< int > pagesWithTextSelection;
    bool mouseOnRect;
                             
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
    // annotations
    PageViewAnnotator * annotator;
    // other stuff
    QTimer * delayResizeTimer;
    bool dirtyLayout;
    bool blockViewport;                 // prevents changes to viewport
    bool blockPixmapsRequest;           // prevent pixmap requests
    PageViewMessage * messageWindow;    // in pageviewutils.h

    // actions
    KSelectAction * aOrientation;
    KSelectAction * aPaperSizes;
    KAction * aMouseNormal;
    KAction * aMouseSelect;
    KAction * aMouseTextSelect;
    KToggleAction * aToggleAnnotator;
    KSelectAction * aZoom;
    KToggleAction * aZoomFitWidth;
    KToggleAction * aZoomFitPage;
    KToggleAction * aZoomFitText;
    KSelectAction * aRenderMode;
    KToggleAction * aViewContinuous;
    KAction * aPrevAction;
    KActionCollection * actionCollection;
};



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
    : QGraphicsView( parent )
{
    // create and initialize private storage structure
    d = new PageViewPrivate();
    d->document = document;
    d->aOrientation = 0;
    d->aRenderMode = 0;
    d->zoomMode = (PageView::ZoomMode) KpdfSettings::zoomMode();
    d->zoomFactor = KpdfSettings::zoomFactor();
    d->mouseMode = MouseNormal;
    d->mouseMidZooming = false;
    d->mouseSelecting = false;
    d->rubberBand = 0;
    d->mouseTextSelecting = false;
    d->mouseOnRect = false;
    d->typeAheadActive = false;
    d->findTimeoutTimer = 0;
    d->viewportMoveActive = false;
    d->viewportMoveTimer = 0;
    d->scrollIncrement = 0;
    d->autoScrollTimer = 0;
    d->annotator = 0;
    d->delayResizeTimer = 0;
    d->dirtyLayout = false;
    d->blockViewport = false;
    d->blockPixmapsRequest = false;
    d->messageWindow = new PageViewMessage(this);
    d->aPrevAction = 0;
/*
    d->mouseTextSelectionRect=0;
    d->mouseTextSelectionInfo=0;
    d->mouseTextSelectionPainted=0;
*/
    d->aPaperSizes=0;

    setObjectName( QLatin1String( "okular::pageView" ) );

    // widget setup: setup focus, accept drops and track mouse
    viewport()->setFocusProxy( this );
    viewport()->setFocusPolicy( Qt::StrongFocus );
    setAcceptDrops( true );
    viewport()->setMouseTracking( true );

    // scene setup
    setScene( new QGraphicsScene() );
    scene()->setBackgroundBrush( QBrush( Qt::gray ) );

    // conntect the padding of the viewport to pixmaps requests
    connect( horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotRequestVisiblePixmaps(int)) );
    connect( verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotRequestVisiblePixmaps(int)) );

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
    // delete the local storage structure
    d->document->removeObserver( this );
    delete d;
}

void PageView::setupActions( KActionCollection * ac )
{
    d->actionCollection = ac;

    d->aOrientation=new KSelectAction( i18n( "&Orientation" ), ac, "view_orientation" );
    d->aPaperSizes=new KSelectAction( i18n( "&Paper sizes" ), ac, "view_papersizes" );
    QStringList rotations;
    rotations.append( i18n( "Default" ) );
    rotations.append( i18n( "Rotated 90 Degrees" ) );
    rotations.append( i18n( "Rotated 180 Degrees" ) );
    rotations.append( i18n( "Rotated 270 Degrees" ) );
    d->aOrientation->setItems( rotations );

    connect( d->aOrientation , SIGNAL( triggered( int ) ),
         d->document , SLOT( slotRotation( int ) ) );
    connect( d->aPaperSizes , SIGNAL( triggered( int ) ),
         d->document , SLOT( slotPaperSizes( int ) ) );

    d->aOrientation->setEnabled(d->document->supportsRotation());
    bool paperSizes=d->document->supportsPaperSizes();
    d->aPaperSizes->setEnabled(paperSizes);
    if (paperSizes)
      d->aPaperSizes->setItems(d->document->paperSizes());

    // Zoom actions ( higher scales takes lots of memory! )
    d->aZoom = new KSelectAction( KIcon( "viewmag" ), i18n( "Zoom" ), ac, "zoom_to" );
    d->aZoom->setEditable( true );
    d->aZoom->setMaxComboViewCount( 13 );
    connect( d->aZoom, SIGNAL( triggered(QAction *) ), this, SLOT( slotZoom() ) );
    updateZoomText();

    KStdAction::zoomIn( this, SLOT( slotZoomIn() ), ac, "zoom_in" );

    KStdAction::zoomOut( this, SLOT( slotZoomOut() ), ac, "zoom_out" );

    d->aZoomFitWidth = new KToggleAction( KIcon( "view_fit_width" ), i18n("Fit to Page &Width"), ac, "zoom_fit_width" );
    connect( d->aZoomFitWidth, SIGNAL( toggled( bool ) ), SLOT( slotFitToWidthToggled( bool ) ) );

    d->aZoomFitPage = new KToggleAction( KIcon( "view_fit_window" ), i18n("Fit to &Page"), ac, "zoom_fit_page" );
    connect( d->aZoomFitPage, SIGNAL( toggled( bool ) ), SLOT( slotFitToPageToggled( bool ) ) );

    d->aZoomFitText = new KToggleAction( KIcon( "viewmagfit" ), i18n("Fit to &Text"), ac, "zoom_fit_text" );
    connect( d->aZoomFitText, SIGNAL( toggled( bool ) ), SLOT( slotFitToTextToggled( bool ) ) );

    // View-Layout actions
    QStringList renderModes;
    renderModes.append( i18n( "Single" ) );
    renderModes.append( i18n( "Facing" ) );
    renderModes.append( i18n( "Overview" ) );

    d->aRenderMode = new KSelectAction( KIcon( "view_left_right" ), i18n("&Render Mode"), ac, "view_render_mode" );
    connect( d->aRenderMode, SIGNAL( triggered( int ) ), SLOT( slotRenderMode( int ) ) );
    d->aRenderMode->setItems( renderModes );
    d->aRenderMode->setCurrentItem( KpdfSettings::renderMode() );

    d->aViewContinuous = new KToggleAction( KIcon( "view_text" ), i18n("&Continuous"), ac, "view_continuous" );
    connect( d->aViewContinuous, SIGNAL( toggled( bool ) ), SLOT( slotContinuousToggled( bool ) ) );
    d->aViewContinuous->setChecked( KpdfSettings::viewContinuous() );

    // Mouse-Mode actions
    QActionGroup * actGroup = new QActionGroup( this );
    actGroup->setExclusive( true );
    d->aMouseNormal = new KAction( KIcon( "mouse" ), i18n("&Browse Tool"), ac, "mouse_drag" );
    connect( d->aMouseNormal, SIGNAL( triggered() ), this, SLOT( slotSetMouseNormal() ) );
    d->aMouseNormal->setCheckable( true );
    d->aMouseNormal->setActionGroup( actGroup );
    d->aMouseNormal->setChecked( true );

    KAction * mz = new KAction( KIcon( "viewmag" ), i18n("&Zoom Tool"), ac, "mouse_zoom" );
    connect( mz, SIGNAL( triggered() ), this, SLOT( slotSetMouseZoom() ) );
    mz->setCheckable( true );
    mz->setActionGroup( actGroup );

    d->aMouseSelect = new KAction( KIcon( "frame_edit" ), i18n("&Select Tool"), ac, "mouse_select" );
    connect( d->aMouseSelect, SIGNAL( triggered() ), this, SLOT( slotSetMouseSelect() ) );
    d->aMouseSelect->setCheckable( true );
    d->aMouseSelect->setActionGroup( actGroup );

    d->aMouseTextSelect = new KAction( KIcon( "text" ), i18n("&Text Selection Tool"), ac, "mouse_textselect" );
    connect( d->aMouseTextSelect, SIGNAL( triggered() ), this, SLOT( slotSetMouseTextSelect() ) );
    d->aMouseTextSelect->setCheckable( true );
    d->aMouseTextSelect->setActionGroup( actGroup );

    d->aToggleAnnotator = new KToggleAction( KIcon( "pencil" ), i18n("&Review"), ac, "mouse_toggle_annotate" );
    d->aToggleAnnotator->setCheckable( true );
    connect( d->aToggleAnnotator, SIGNAL( toggled( bool ) ), SLOT( slotToggleAnnotator( bool ) ) );
    d->aToggleAnnotator->setShortcut( Qt::Key_F6 );

    // Other actions
    KAction * su = new KAction( i18n("Scroll Up"), ac, "view_scroll_up" );
    connect( su, SIGNAL( triggered() ), this, SLOT( slotScrollUp() ) );
    su->setShortcut( QKeySequence(Qt::SHIFT + Qt::Key_Up) );

    KAction * sd = new KAction( i18n("Scroll Down"), ac, "view_scroll_down" );
    connect( sd, SIGNAL( triggered() ), this, SLOT( slotScrollDown() ) );
    sd->setShortcut( QKeySequence(Qt::SHIFT + Qt::Key_Down) );
}

bool PageView::canFitPageWidth()
{
    return KpdfSettings::renderMode() != 0 || d->zoomMode != ZoomFitWidth;
}

void PageView::fitPageWidth( int page )
{
    // zoom: Fit Width, columns: 1. setActions + relayout + setPage + update
    d->zoomMode = ZoomFitWidth;
    KpdfSettings::setRenderMode( 0 );
    d->aZoomFitWidth->setChecked( true );
    d->aZoomFitPage->setChecked( false );
    d->aZoomFitText->setChecked( false );
    d->aRenderMode->setCurrentItem( 0 );
    viewport()->setUpdatesEnabled( false );
    slotRelayoutPages();
    viewport()->setUpdatesEnabled( true );
    d->document->setViewportPage( page );
    updateZoomText();
    setFocus();
}

void PageView::displayMessage( const QString & message,PageViewMessage::Icon icon,int duration )
{
    if ( !KpdfSettings::showOSD() )
    {
        if (icon == PageViewMessage::Error)
            KMessageBox::error( this, message );
        else
            return;
    }

    // hide messageWindow if string is empty
    if ( message.isEmpty() )
        return d->messageWindow->hide();

    // display message (duration is length dependant)
    if (duration==-1)
        duration = 500 + 100 * message.length();
    d->messageWindow->display( message, icon, duration );
}

//BEGIN DocumentObserver inherited methods
void PageView::notifySetup( const QVector< KPDFPage * > & pageSet, bool documentChanged )
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
    QVector< PageViewItem * >::iterator dIt = d->items.begin(), dEnd = d->items.end();
    for ( ; dIt != dEnd; ++dIt )
    {
        scene()->removeItem( *dIt );
        delete *dIt;
    }
    d->items.clear();
    d->visibleItems.clear();

    // create children widgets
    QVector< KPDFPage * >::const_iterator setIt = pageSet.begin(), setEnd = pageSet.end();
    for ( ; setIt != setEnd; ++setIt )
    {
        PageViewItem * newitem = new PageViewItem( *setIt );
        d->items.push_back( newitem );
        scene()->addItem( newitem );
        newitem->setAnnotator( d->annotator );
#ifdef PAGEVIEW_DEBUG
        kDebug() << "geom for " << newitem->pageNumber() << " is " << newitem->geometry() << endl;
#endif
    }

    // invalidate layout so relayout/repaint will happen on next viewport change
    if ( pageSet.count() > 0 )
        // TODO for Enrico: Check if doing always the slotRelayoutPages() is not
        // suboptimal in some cases, i'd say it is not but a recheck will not hurt
        // Need slotRelayoutPages() here instead of d->dirtyLayout = true
        // because opening a pdf from another pdf will not trigger a viewportchange
        // so pages are never relayouted
        QTimer::singleShot(0, this, SLOT(slotRelayoutPages())); // was used
    else
    {
        // update the mouse cursor when closing because we may have close through a link and
        // want the cursor to come back to the normal cursor
        updateCursor( mapFromGlobal( QCursor::pos() ) + mapToScene( 0, 0 ).toPoint() );
        setSceneRect( 0, 0, 0, 0 );
    }

    // OSD to display pages
    if ( documentChanged && pageSet.count() > 0 && KpdfSettings::showOSD() )
        d->messageWindow->display(
            i18np(" Loaded a one-page document.",
                 " Loaded a %n-page document.",
                 pageSet.count() ),
            PageViewMessage::Info, 4000 );

    d->aOrientation->setEnabled(d->document->supportsRotation());
    bool paperSizes=d->document->supportsPaperSizes();
    d->aPaperSizes->setEnabled(paperSizes);
    // set the new paper sizes:
    // - if the generator supports them
    // - if the document changed
    if (paperSizes && documentChanged)
      d->aPaperSizes->setItems(d->document->paperSizes());
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
    QVector< PageViewItem * >::iterator iIt = d->items.begin(), iEnd = d->items.end();
    for ( ; iIt != iEnd; ++iIt )
        if ( (*iIt)->pageNumber() == vp.pageNumber )
        {
            item = *iIt;
            break;
        }
    if ( !item )
    {
        kDebug() << "viewport has no matching item!" << endl;
        d->blockViewport = false;
        return;
    }
#ifdef PAGEVIEW_DEBUG
    kDebug() << "document viewport changed\n";
#endif
    // relayout in "Single Pages" mode or if a relayout is pending
    d->blockPixmapsRequest = true;
    if ( !KpdfSettings::viewContinuous() || d->dirtyLayout )
        slotRelayoutPages();

    // restore viewport center or use default {x-center,v-top} alignment
    const QRectF & r = item->geometry();
    double newCenterX = r.left(),
        newCenterY = r.top();
    if ( vp.rePos.enabled )
    {
        if ( vp.rePos.pos == DocumentViewport::Center )
        {
            newCenterX += (int)( vp.rePos.normalizedX * r.width() );
            newCenterY += (int)( vp.rePos.normalizedY * r.height() );
        }
        else
        {
            // TopLeft
            newCenterX += (int)( vp.rePos.normalizedX * r.width() + viewport()->width() / 2 );
            newCenterY += (int)( vp.rePos.normalizedY * r.height() + viewport()->height() / 2 );
        }
    }
    else
    {
        newCenterX += r.width() / 2;
        newCenterY += viewport()->rect().height() / 2 - 10;
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
        centerOn( newCenterX, newCenterY );
    d->blockPixmapsRequest = false;

    // request visible pixmaps in the current viewport and recompute it
    slotRequestVisiblePixmaps();

    // enable setViewport calls
    d->blockViewport = false;

    // update zoom text if in a ZoomFit/* zoom mode
    if ( d->zoomMode != ZoomFixed )
        updateZoomText();

    // since the page has moved below cursor, update it
    updateCursor( mapFromGlobal( QCursor::pos() ) + mapToScene( 0, 0 ).toPoint() );
}

void PageView::notifyPageChanged( int pageNumber, int changedFlags )
{
    // only handle pixmap / highlight changes notifies
    if ( changedFlags & DocumentObserver::Bookmark )
        return;

    // iterate over visible items: if page(pageNumber) is one of them, repaint it
    QLinkedList< PageViewItem * >::iterator iIt = d->visibleItems.begin(), iEnd = d->visibleItems.end();
    for ( ; iIt != iEnd; ++iIt )
        if ( (*iIt)->pageNumber() == pageNumber )
        {
/*
            // update item's rectangle plus the little outline
            QRectF expandedRect = (*iIt)->geometry();
            expandedRect.adjust( -1, -1, 3, 3 );
*/
            (*iIt)->update();

            // if we were "zoom-dragging" do not overwrite the "zoom-drag" cursor
            if ( cursor().shape() != Qt::SizeVerCursor )
            {
                // since the page has been regenerated below cursor, update it
                updateCursor( mapFromGlobal( QCursor::pos() ) + mapToScene( 0, 0 ).toPoint() );
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
    QLinkedList< PageViewItem * >::iterator vIt = d->visibleItems.begin(), vEnd = d->visibleItems.end();
    for ( ; vIt != vEnd; ++vIt )
        if ( (*vIt)->pageNumber() == pageNumber )
            return false;
    // if hidden premit unloading
    return true;
}
//END DocumentObserver inherited methods

//BEGIN widget events
/*
void PageView::paintEvent( QPaintEvent * pe )
{
if ( d->document->handleEvent( pe ) )
{
    // create the rect into contents from the clipped screen rect
    QRect viewportRect = viewport()->rect();
    QRect contentsRect = pe->rect().intersect( viewportRect );
    contentsRect.translate( contentsX(), contentsY() );
    if ( !contentsRect.isValid() )
        return;

    // create the screen painter. a pixel painted at contentsX,contentsY
    // appears to the top-left corner of the scrollview.
    QPainter screenPainter( viewport() );
    screenPainter.translate( -contentsX(), -contentsY() );

    // selectionRect is the normalized mouse selection rect
    QRect selectionRect = d->mouseSelectionRect;
    if ( !selectionRect.isNull() )
        selectionRect = selectionRect.normalized();
    // selectionRectInternal without the border
    QRect selectionRectInternal = selectionRect;
    selectionRectInternal.adjust( 1, 1, -1, -1 );
    // color for blending
    QColor selBlendColor = (selectionRect.width() > 8 || selectionRect.height() > 8) ?
                           d->mouseSelectionColor : Qt::red;

    // subdivide region into rects
    QVector<QRect> allRects = pe->region().rects();
    int numRects = allRects.count();

    // preprocess rects area to see if it worths or not using subdivision
    uint summedArea = 0;
    for ( int i = 0; i < numRects; i++ )
    {
        const QRect & r = allRects[i];
        summedArea += r.width() * r.height();
    }
    // very elementary check: SUMj(Region[j].area) is less than boundingRect.area
    bool useSubdivision = summedArea < (0.6 * contentsRect.width() * contentsRect.height());
    if ( !useSubdivision )
        numRects = 1;

    // iterate over the rects (only one loop if not using subdivision)
    for ( uint i = 0; i < numRects; i++ )
    {
        if ( useSubdivision )
        {
            // set 'contentsRect' to a part of the sub-divided region
            contentsRect = allRects[i].normalized().intersect( viewportRect );
            contentsRect.translate( contentsX(), contentsY() );
            if ( !contentsRect.isValid() )
                continue;
        }

        // note: this check will take care of all things requiring alpha blending (not only selection)
        bool wantCompositing = ( !selectionRect.isNull() && contentsRect.intersects( selectionRect ) )
            || d->mouseTextSelecting;

        if ( wantCompositing && KpdfSettings::enableCompositing() )
        {
            // create pixmap and open a painter over it (contents{left,top} becomes pixmap {0,0})
            QPixmap doubleBuffer( contentsRect.size() );
            QPainter pixmapPainter( &doubleBuffer );
            pixmapPainter.translate( -contentsRect.left(), -contentsRect.top() );

            // calculate the color
            XRenderColor col;
            float alpha=0.2f;
            QColor blCol=selBlendColor.dark(140);
            col.red=( (blCol.red() << 8) | blCol.red() ) * alpha;
            col.green=( (blCol.green() << 8) | blCol.green() )*alpha;
            col.blue=( (blCol.blue() << 8) | blCol.blue())*alpha;
            col.alpha=alpha*0xffff;

            // 1) Layer 0: paint items and clear bg on unpainted rects
            drawDocumentOnPainter( contentsRect, &pixmapPainter );
            // 2) Layer 1a: paint (blend) transparent selection
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
//                     QImage blendedImage = blendedPixmap.convertToImage();
//                     KImageEffect::blend( selBlendColor.dark(140), blendedImage, 0.2 );
                    XRenderFillRectangle(x11Info().display(), PictOpOver, blendedPixmap.x11PictureHandle(), &col, 
                      0,0, blendRect.width(), blendRect.height());
                    // copy the blended pixmap back to its place
                    pixmapPainter.drawPixmap( blendRect.left(), blendRect.top(), blendedPixmap );
                }
                // draw border (red if the selection is too small)
                pixmapPainter.setPen( selBlendColor );
                pixmapPainter.drawRect( selectionRect );
            }
            if ( d->mouseTextSelecting )
            {
              QRect blendRect;
              QList<QRect>::iterator it=d->mouseTextSelectionRect->begin(),
              end=d->mouseTextSelectionRect->end();
              XRenderColor col;
              float alpha=0.2f;
              QColor blCol=d->mouseTextSelectionColor.dark(140);
              col.red=( (blCol.red() << 8) | blCol.red() ) * alpha;
              col.green=( (blCol.green() << 8) | blCol.green() )*alpha;
              col.blue=( (blCol.blue() << 8) | blCol.blue())*alpha;
              col.alpha=alpha*0xffff;

              for (;it!=end;++it)
              {
                if (! ((*it).intersects( contentsRect )))
                  continue;
                blendRect = (*it).intersect(contentsRect);
                QPixmap blendedPixmap( blendRect.width(), blendRect.height() );
                copyBlt( &blendedPixmap, 0,0, &doubleBuffer,
                          blendRect.left() - contentsRect.left(), blendRect.top() - contentsRect.top(),
                          blendRect.width(), blendRect.height() );
                    // blend selBlendColor into the background pixmap
                XRenderFillRectangle(x11Info().display(), PictOpOver, blendedPixmap.x11PictureHandle(), &col, 
                  0,0, blendRect.width(), blendRect.height());

//                 KImageEffect::blend( d->mouseTextSelectionColor.dark(140), blendedImage, 0.2 );

                // copy the blended pixmap back to its place
                pixmapPainter.drawPixmap( blendRect.left(), blendRect.top(), blendedPixmap );
              
                // draw border (red if the selection is too small)
                pixmapPainter.setPen( d->mouseTextSelectionColor );
                pixmapPainter.drawRect( selectionRect );
              }
                 
              
            }
            // 3) Layer 1: give annotator painting control
            if ( d->annotator && d->annotator->routePaints( contentsRect ) )
                d->annotator->routePaint( &pixmapPainter, contentsRect );
            // 4) Layer 2: overlays
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
            drawDocumentOnPainter( contentsRect, &screenPainter );
            // 2) Layer 1: paint opaque selection
            if ( !selectionRect.isNull() && selectionRect.intersects( contentsRect ) &&
                 !selectionRectInternal.contains( contentsRect ) )
            {
                screenPainter.setPen( palette().color( QPalette::Active, QPalette::Highlight ).dark(110) );
                screenPainter.drawRect( selectionRect );
            }
            // 3) Layer 1: give annotator painting control
            if ( d->annotator && d->annotator->routePaints( contentsRect ) )
                d->annotator->routePaint( &screenPainter, contentsRect );
            // 4) Layer 2: overlays
            if ( KpdfSettings::debugDrawBoundaries() )
            {
                screenPainter.setPen( Qt::red );
                screenPainter.drawRect( contentsRect );
            }
        }
    }
}
}
*/

void PageView::resizeEvent( QResizeEvent * event)
{
if (d->document->handleEvent( event ) )
{
    if ( d->items.isEmpty() )
        return;

    // start a timer that will refresh the pixmap after 0.2s
    if ( !d->delayResizeTimer )
    {
        d->delayResizeTimer = new QTimer( this );
        d->delayResizeTimer->setSingleShot( true );
        connect( d->delayResizeTimer, SIGNAL( timeout() ), this, SLOT( slotRelayoutPages() ) );
    }
    d->delayResizeTimer->start( 200 );
}
}

void PageView::keyPressEvent( QKeyEvent * e )
{
if (d->document->handleEvent( e ) )
{
    e->accept();

    // if performing a selection or dyn zooming, disable keys handling
    if ( d->mouseSelecting || d->mouseMidZooming )
        return;

    // handle 'find as you type' (based on khtml/khtmlview.cpp)
    if( d->typeAheadActive )
    {
        // backspace: remove a char and search or terminates search
        if( e->key() == Qt::Key_Backspace )
        {
            if( d->typeAheadString.length() > 1 )
            {
                d->typeAheadString = d->typeAheadString.left( d->typeAheadString.length() - 1 );
                bool found = d->document->searchText( PAGEVIEW_SEARCH_ID, d->typeAheadString, true, false,
                        KPDFDocument::NextMatch, true, qRgb( 128, 255, 128 ), true );
                KLocalizedString status = found ? ki18n("Text found: \"%1\".") : ki18n("Text not found: \"%1\".");
                d->messageWindow->display( status.subs(d->typeAheadString.toLower()).toString(),
                                           found ? PageViewMessage::Find : PageViewMessage::Warning, 4000 );
                d->findTimeoutTimer->start( 3000 );
            }
            else
            {
                slotStopFindAhead();
                d->document->resetSearch( PAGEVIEW_SEARCH_ID );
            }
        }
        // go to next occurrency
        else if( e->key() == d->actionCollection->action( "find_next" )->shortcut().keyQt() )
        {
            // part doesn't get this key event because of the keyboard grab
            d->findTimeoutTimer->stop(); // restore normal operation during possible messagebox is displayed
            // (1/4) it is needed to grab the keyboard becase people may have Space assigned
            // to a accel and without grabbing the keyboard you can not vim-search for space
            // because it activates the accel
            releaseKeyboard();
            if ( d->document->continueSearch( PAGEVIEW_SEARCH_ID ) )
                d->messageWindow->display( i18n("Text found: \"%1\".", d->typeAheadString.toLower()),
                                           PageViewMessage::Find, 3000 );
            d->findTimeoutTimer->start( 3000 );
            // (2/4) it is needed to grab the keyboard becase people may have Space assigned
            // to a accel and without grabbing the keyboard you can not vim-search for space
            // because it activates the accel
            grabKeyboard();
        }
        // esc and return: end search
        else if( e->key() == Qt::Key_Escape || e->key() == Qt::Key_Return )
        {
            slotStopFindAhead();
        }
        // other key: add to text and search
        else if( !e->text().isEmpty() )
        {
            d->typeAheadString += e->text();
            bool found = d->document->searchText( PAGEVIEW_SEARCH_ID, d->typeAheadString, false, false,
                    KPDFDocument::NextMatch, true, qRgb( 128, 255, 128 ), true );
            KLocalizedString status = found ? ki18n("Text found: \"%1\".") : ki18n("Text not found: \"%1\".");
            d->messageWindow->display( status.subs(d->typeAheadString.toLower()).toString(),
                                       found ? PageViewMessage::Find : PageViewMessage::Warning, 4000 );
            d->findTimeoutTimer->start( 3000 );
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
            d->findTimeoutTimer->setSingleShot( true );
            connect( d->findTimeoutTimer, SIGNAL( timeout() ), this, SLOT( slotStopFindAhead() ) );
        }
        d->findTimeoutTimer->start( 3000 );
        // (3/4) it is needed to grab the keyboard becase people may have Space assigned
        // to a accel and without grabbing the keyboard you can not vim-search for space
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
        case Qt::Key_Up:
        case Qt::Key_PageUp:
        case Qt::Key_Backspace:
            // if in single page mode and at the top of the screen, go to \ page
            if ( KpdfSettings::viewContinuous() || verticalScrollBar()->value() > verticalScrollBar()->minimum() )
            {
                if ( e->key() == Qt::Key_Up )
                    verticalScrollBar()->triggerAction( QScrollBar::SliderSingleStepSub );
                else
                    verticalScrollBar()->triggerAction( QScrollBar::SliderPageStepSub );
            }
            else if ( d->document->currentPage() > 0 )
            {
                // more optimized than document->setPrevPage and then move view to bottom
                DocumentViewport newViewport = d->document->viewport();
                newViewport.pageNumber -= viewColumns();
                if ( newViewport.pageNumber < 0 )
                    newViewport.pageNumber = 0;
                newViewport.rePos.enabled = true;
                newViewport.rePos.normalizedY = 1.0;
                d->document->setViewport( newViewport );
            }
            break;
        case Qt::Key_Down:
        case Qt::Key_PageDown:
        case Qt::Key_Space:
            // if in single page mode and at the bottom of the screen, go to next page
            if ( KpdfSettings::viewContinuous() || verticalScrollBar()->value() < verticalScrollBar()->maximum() )
            {
                if ( e->key() == Qt::Key_Down )
                    verticalScrollBar()->triggerAction( QScrollBar::SliderSingleStepAdd );
                else
                    verticalScrollBar()->triggerAction( QScrollBar::SliderPageStepAdd );
            }
            else if ( d->document->currentPage() < d->items.count() - 1 )
            {
                // more optimized than document->setNextPage and then move view to top
                DocumentViewport newViewport = d->document->viewport();
                newViewport.pageNumber += d->document->currentPage() ? viewColumns() : 1;
                if ( newViewport.pageNumber >= (int)d->items.count() )
                    newViewport.pageNumber = d->items.count() - 1;
                newViewport.rePos.enabled = true;
                newViewport.rePos.normalizedY = 0.0;
                d->document->setViewport( newViewport );
            }
            break;
        case Qt::Key_Left:
            horizontalScrollBar()->triggerAction( QScrollBar::SliderSingleStepSub );
            break;
        case Qt::Key_Right:
            horizontalScrollBar()->triggerAction( QScrollBar::SliderSingleStepAdd );
            break;
        case Qt::Key_Shift:
        case Qt::Key_Control:
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
}

void PageView::mouseMoveEvent( QMouseEvent * e )
{
if (d->document->handleEvent( e ) )
{
    // don't perform any mouse action when no document is shown
    if ( d->items.isEmpty() )
        return;

    // don't perform any mouse action when viewport is autoscrolling
    if ( d->viewportMoveActive )
        return;

    // if holding mouse mid button, perform zoom
    if ( d->mouseMidZooming && (e->state() & Qt::MidButton) )
    {
        int mouseY = e->globalPos().y();
        int deltaY = d->mouseMidLastY - mouseY;

        // wrap mouse from top to bottom
        QRect mouseContainer = KGlobalSettings::desktopGeometry( this );
        if ( mouseY <= mouseContainer.top() + 4 &&
             d->zoomFactor < 3.99 )
        {
            mouseY = mouseContainer.bottom() - 5;
            QCursor::setPos( e->globalPos().x(), mouseY );
        }
        // wrap mouse from bottom to top
        else if ( mouseY >= mouseContainer.bottom() - 4 &&
                  d->zoomFactor > 0.11 )
        {
            mouseY = mouseContainer.top() + 5;
            QCursor::setPos( e->globalPos().x(), mouseY );
        }
        // remember last position
        d->mouseMidLastY = mouseY;

        // update zoom level, perform zoom and redraw
        if ( deltaY )
        {
            d->zoomFactor *= ( 1.0 + ( (double)deltaY / 500.0 ) );
            updateZoom( ZoomRefreshCurrent );
            viewport()->repaint();
        }
        return;
    }

    // if we're editing an annotation, dispatch event to it
    if ( d->annotator && d->annotator->routeEvents() )
    {
        QPointF mapped = mapToScene( 0, 0 ) + e->pos();
        PageViewItem * pageItem = pickItemOnPoint( (int)mapped.x(), (int)mapped.y() );
        if ( pageItem )
        {
            QRect updated = d->annotator->routeEvent( e, mapped, pageItem );
            if ( !updated.isEmpty() )
                pageItem->update( QRectF( updated ) );
        }
        return;
    }

    bool leftButton = e->state() & Qt::LeftButton,
         rightButton = e->state() & Qt::RightButton;
    switch ( d->mouseMode )
    {
        case MouseNormal:
            if ( leftButton )
            {
                // drag page
                if ( !d->mouseGrabPos.isNull() )
                {
                    QPoint mousePos = e->globalPos();
                    QPoint delta = d->mouseGrabPos - mousePos;

                    // wrap mouse from top to bottom
                    QRect mouseContainer = KGlobalSettings::desktopGeometry( this );
                    if ( mousePos.y() <= mouseContainer.top() + 4 &&
                         verticalScrollBar()->value() < verticalScrollBar()->maximum() - 10 )
                    {
                        mousePos.setY( mouseContainer.bottom() - 5 );
                        QCursor::setPos( mousePos );
                    }
                    // wrap mouse from bottom to top
                    else if ( mousePos.y() >= mouseContainer.bottom() - 4 &&
                              verticalScrollBar()->value() > 10 )
                    {
                        mousePos.setY( mouseContainer.top() + 5 );
                        QCursor::setPos( mousePos );
                    }
                    // remember last position
                    d->mouseGrabPos = mousePos;

                    // scroll page by position increment
                    scrollContentsBy( -delta.x(), -delta.y() );
                }
            }
/*
*/
            else
            {
                // only hovering the page, so update the cursor
                updateCursor( e->pos() + mapToScene( 0, 0 ).toPoint() );
            }
            break;

        case MouseZoom:
        case MouseSelect:
            // set second corner of selection
            if ( d->rubberBand && d->mouseSelecting )
            {
                d->rubberBand->resizeTo( mapToScene( 0, 0 ) + e->pos() );
            }
            break;
        case MouseTextSelect:
            // if mouse moves 5 px away from the press point and the document soupports text extraction, do 'textselection'
            if ( !d->mouseTextSelecting && !d->mousePressPos.isNull() && d->document->supportsSearching() && ( ( e->pos() - d->mouseSelectPos ).manhattanLength() > 5 ) )
            {
                d->mouseTextSelecting = true;
            }
            if ( d->mouseTextSelecting )
            {
                QSet< int > affectedItemsSet;
                QRect selectionRect = QRect( e->pos(), d->mouseSelectPos ).normalized();
                foreach( QGraphicsItem * item, items( selectionRect ) )
                {
                    PageViewItem * pageItem = qgraphicsitem_cast< PageViewItem * >( item );
                    if ( !pageItem ) continue;

                    affectedItemsSet.insert( pageItem->pageNumber() );
                }
                kDebug() << ">>>> item selected by mouse: " << affectedItemsSet.count() << endl;
                QSet< int > pagesWithSelectionSet;

                if ( !affectedItemsSet.isEmpty() )
                {
                    // is the mouse drag line the ne-sw diagonal of the selection rect?
                    bool direction_ne_sw = e->pos() == selectionRect.topRight() || e->pos() == selectionRect.bottomLeft();

                    int tmpmin = d->document->pages();
                    int tmpmax = 0;
                    foreach( int p, affectedItemsSet )
                    {
                        if ( p < tmpmin ) tmpmin = p;
                        if ( p > tmpmax ) tmpmax = p;
                    }

                    PageViewItem * a = pickItemOnPoint( (int)( direction_ne_sw ? selectionRect.right() : selectionRect.left() ), (int)selectionRect.top() );
                    int min = a && ( a->pageNumber() != tmpmax ) ? a->pageNumber() : tmpmin;
                    PageViewItem * b = pickItemOnPoint( (int)( direction_ne_sw ? selectionRect.left() : selectionRect.right() ), (int)selectionRect.bottom() );
                    int max = b && ( b->pageNumber() != tmpmin ) ? b->pageNumber() : tmpmax;

                    QList< int > affectedItemsIds;
                    for ( int i = min; i <= max; ++i )
                        affectedItemsIds.append( i );
                    kDebug() << ">>>> pages: " << affectedItemsIds << endl;

                    // transform the selection rect coords into scene coords
                    selectionRect.translate( mapToScene( 0, 0 ).toPoint() );

                    if ( affectedItemsIds.count() == 1 )
                    {
                        PageViewItem * item = d->items[ affectedItemsIds.first() ];
                        selectionRect.translate( -item->pos().toPoint() );
                        textSelectionForItem( item,
                            direction_ne_sw ? selectionRect.topRight() : selectionRect.topLeft(),
                            direction_ne_sw ? selectionRect.bottomLeft() : selectionRect.bottomRight() );
                        pagesWithSelectionSet.insert( affectedItemsIds.first() );
                    }
                    else if ( affectedItemsIds.count() > 1 )
                    {
                        // first item
                        PageViewItem * first = d->items[ affectedItemsIds.first() ];
                        QRect geom = first->geometry().intersect( selectionRect ).translated( -first->pos() ).toRect();
                        textSelectionForItem( first,
                            selectionRect.bottom() > geom.height() ? ( direction_ne_sw ? geom.topRight() : geom.topLeft() ) : ( direction_ne_sw ? geom.bottomRight() : geom.bottomLeft() ),
                            QPoint() );
                        pagesWithSelectionSet.insert( affectedItemsIds.first() );
                        // last item
                        PageViewItem * last = d->items[ affectedItemsIds.last() ];
                        geom = last->geometry().intersect( selectionRect ).translated( -last->pos() ).toRect();
                        textSelectionForItem( last,
                            QPoint(),
//                            direction_ne_sw ? geom.bottomLeft() : geom.bottomRight() );
                            selectionRect.bottom() > geom.height() ? ( direction_ne_sw ? geom.bottomLeft() : geom.bottomRight() ) : ( direction_ne_sw ? geom.topLeft() : geom.topRight() ) );
                        pagesWithSelectionSet.insert( affectedItemsIds.last() );
                        affectedItemsIds.removeFirst();
                        affectedItemsIds.removeLast();
                        // item between the two above
                        foreach( int page, affectedItemsIds )
                        {
                            textSelectionForItem( d->items[ page ] );
                            pagesWithSelectionSet.insert( page );
                        }
                    }
                }
                QSet< int > noMoreSelectedPages = d->pagesWithTextSelection - pagesWithSelectionSet;
                // clear the selection from pages not selected anymore
                foreach( int p, noMoreSelectedPages )
                {
                    d->document->setPageTextSelection( p, 0, QColor() );
                }
                d->pagesWithTextSelection = pagesWithSelectionSet;
            }

/*
                    PageViewItem * currentItem = d->items[ qMax( 0, (int)d->document->currentPage() ) ];
//                     PageViewItem* item=pickItemOnPoint(e->x(),e->y());
                    const KPDFPage * kpdfPage = currentItem->page();
                    // build a proper rectangle (make sure left/top is to the left of right/bottom)
//                     QRect rect (d->mouseSelectPos,e->pos());
//                     rect=rect.normalize();
//                     
                    QRect vRect = currentItem->geometry().toRect();
//                     kDebug() << "viewport " << vRect << endl;
//                     kDebug() << "selection (UN) " << rect << endl;
//                     // move selection area over to relevant viewport
//                     rect.moveBy(-vRect.left(),-vRect.top()); 
//                     kDebug() << "selection (MV) " << rect << endl;
                    // clip to viewport
//                     rect &= vRect;
//                     kDebug() << "selection (CL) " << rect << endl;
//                     FIXME: width and height are greater by 1 then the selection.
//                     rect.addCoords(1,1,-1,-1);
                                        
                    if ( !kpdfPage->hasSearchPage() )
                    	 d->document->requestTextPage( kpdfPage->number() );
                    NormalizedPoint startCursor(d->mouseSelectPos.x()-vRect.left(),d->mouseSelectPos.y()-vRect.top(),
                                vRect.width(), vRect.height());
                    NormalizedPoint endCursor(e->x()-vRect.left(),e->y()-vRect.top(),vRect.width(), vRect.height());

                    if ( ! d->mouseTextSelectionInfo )
                      d->mouseTextSelectionInfo=new ::TextSelection(startCursor,endCursor);
                    else
                      d->mouseTextSelectionInfo->end(endCursor);
		    RegularAreaRect * selectionArea=kpdfPage->getTextArea(d->mouseTextSelectionInfo);
                    kWarning () << "text areas: " << selectionArea->count() << endl;
                    if ( selectionArea->count() > 0 )
                    { 
                      setCursor( Qt::IBeamCursor );
                      QColor selColor = palette().color( QPalette::Active, QPalette::Highlight );
/*
                      textSelection(selectionArea->geometry(vRect.width(),vRect.height(),vRect.left(),vRect.top())
                          ,selColor);
*/
/*
                        d->document->setPageTextSelection( kpdfPage->number(), selectionArea, selColor );
                    }                    
                    delete selectionArea;
                }
            }
*/
            break;
    }
}
}

void PageView::mousePressEvent( QMouseEvent * e )
{
if ( d->document->handleEvent( e ) )
{
    // don't perform any mouse action when no document is shown
    if ( d->items.isEmpty() )
        return;

    // if performing a selection or dyn zooming, disable mouse press
    if ( d->mouseSelecting || d->mouseMidZooming || d->viewportMoveActive )
        return;

    // if the page is scrolling, stop it
    if ( d->autoScrollTimer )
    {
        d->scrollIncrement = 0;
        d->autoScrollTimer->stop();
    }

    // if pressing mid mouse button while not doing other things, begin 'continuous zoom' mode
    if ( e->button() == Qt::MidButton )
    {
        d->mouseMidZooming = true;
        d->mouseMidLastY = e->globalPos().y();
        setCursor( Qt::SizeVerCursor );
        return;
    }

    // if we're editing an annotation, dispatch event to it
    if ( d->annotator && d->annotator->routeEvents() )
    {
        QPointF mapped = mapToScene( 0, 0 ) + e->pos();
        PageViewItem * pageItem = pickItemOnPoint( (int)mapped.x(), (int)mapped.y() );
        if ( pageItem )
        {
            QRect updated = d->annotator->routeEvent( e, mapped, pageItem );
            if ( !updated.isEmpty() )
                pageItem->update( QRectF( updated ) );
        }
        return;
    }

    // update press / 'start drag' mouse position
    d->mousePressPos = e->globalPos();

    // handle mode dependant mouse press actions
    bool leftButton = e->button() == Qt::LeftButton,
         rightButton = e->button() == Qt::RightButton;
        
//   Not sure we should erase the selection when clicking with left.
     if ( d->mouseMode != MouseTextSelect )
       textSelectionClear();
    
    switch ( d->mouseMode )
    {
        case MouseNormal:   // drag start / click / link following
            if ( leftButton )
            {
                d->mouseGrabPos = d->mouseOnRect ? QPoint() : d->mousePressPos;
                if ( !d->mouseOnRect )
                    setCursor( Qt::SizeAllCursor );
            }
            break;
        case MouseZoom:     // set first corner of the zoom rect
            if ( leftButton )
                selectionStart( e->pos(), palette().color( QPalette::Active, QPalette::Highlight ), false );
            else if ( rightButton )
                updateZoom( ZoomOut );
            break;
        case MouseSelect:   // set first corner of the selection rect
//             if ( leftButton )
//             {
                selectionStart( e->pos(), palette().color( QPalette::Active, QPalette::Highlight ).light( 120 ), false );
//             }
            break;
        case MouseTextSelect:
            d->mouseSelectPos = e->pos();
            if ( !rightButton )
            {
                textSelectionClear();
            }
            break;
    }
}
}

void PageView::mouseReleaseEvent( QMouseEvent * e )
{
if (d->document->handleEvent( e ) )
{
    // don't perform any mouse action when no document is shown..
    if ( d->items.isEmpty() )
    {
        // ..except for right Clicks (emitted even it viewport is empty)
        if ( e->button() == Qt::RightButton )
            emit rightClick( 0, e->globalPos() );
        return;
    }

    // don't perform any mouse action when viewport is autoscrolling
    if ( d->viewportMoveActive )
        return;

    // handle mode indepent mid buttom zoom
    if ( d->mouseMidZooming && (e->button() == Qt::MidButton) )
    {
        d->mouseMidZooming = false;
        // request pixmaps since it was disabled during drag
        slotRequestVisiblePixmaps();
        // the cursor may now be over a link.. update it
        updateCursor( e->pos() + mapToScene( 0, 0 ).toPoint() );
        return;
    }

    // if we're editing an annotation, dispatch event to it
    if ( d->annotator && d->annotator->routeEvents() )
    {
        QPointF mapped = mapToScene( 0, 0 ) + e->pos();
        PageViewItem * pageItem = pickItemOnPoint( (int)mapped.x(), (int)mapped.y() );
        if ( pageItem )
        {
            QRect updated = d->annotator->routeEvent( e, mapped, pageItem );
            if ( !updated.isEmpty() )
                pageItem->update( QRectF( updated ) );
        }
        return;
    }

    bool leftButton = e->button() == Qt::LeftButton,
         rightButton = e->button() == Qt::RightButton;
    switch ( d->mouseMode )
    {
        case MouseNormal:{
            // return the cursor to its normal state after dragging
            if ( cursor().shape() == Qt::SizeAllCursor )
                updateCursor( e->pos() + mapToScene( 0, 0 ).toPoint() );

            QPoint mapped = e->pos() + mapToScene( 0, 0 ).toPoint();
            PageViewItem * pageItem = pickItemOnPoint( mapped.x(), mapped.y() );

            // if the mouse has not moved since the press, that's a -click-
            if ( leftButton && pageItem && d->mousePressPos == e->globalPos())
            {
                double nX = (double)(mapped.x() - pageItem->geometry().left()) / (double)pageItem->width(),
                       nY = (double)(mapped.y() - pageItem->geometry().top()) / (double)pageItem->height();
                const ObjectRect * rect;
                rect = pageItem->page()->getObjectRect( ObjectRect::Link, nX, nY );
                if ( rect )
                {
                    // handle click over a link
                    const KPDFLink * link = static_cast< const KPDFLink * >( rect->pointer() );
                    d->document->processLink( link );
                }
                else
                {
                    // a link can move us to another page or even to another document, there's no point in trying to
                    //  process the click on the image once we have processes the click on the link
                    rect = pageItem->page()->getObjectRect( ObjectRect::Image, nX, nY );
                    if ( rect )
                    {
                        // handle click over a image
                    }
/*		Enrico and me have decided this is not worth the trouble it generates
                    else
                    {
                        // if not on a rect, the click selects the page
                        // if ( pageItem->pageNumber() != (int)d->document->currentPage() )
                        d->document->setViewportPage( pageItem->pageNumber(), PAGEVIEW_ID );
                    }*/
                }
            }
            else if ( rightButton )
            {
                if ( pageItem && d->mousePressPos == e->globalPos() )
                {
                    double nX = (double)(mapped.x() - pageItem->geometry().left()) / (double)pageItem->width(),
                           nY = (double)(mapped.y() - pageItem->geometry().top()) / (double)pageItem->height();
                    const ObjectRect * rect;
                    rect = pageItem->page()->getObjectRect( ObjectRect::Link, nX, nY );
                    if ( rect )
                    {
                        // handle right click over a link
                        const KPDFLink * link = static_cast< const KPDFLink * >( rect->pointer() );
                        // creating the menu and its actions
                        KMenu menu( this );
                        QAction * actProcessLink = menu.addAction( i18n( "Follow This Link" ) );
                        QAction * actCopyLinkLocation = 0;
                        if ( dynamic_cast< const KPDFLinkBrowse * >( link ) )
                            actCopyLinkLocation = menu.addAction( i18n( "Copy Link Location" ) );
                        QAction * res = menu.exec( e->globalPos() );
                        if ( res )
                        {
                            if ( res == actProcessLink )
                            {
                                d->document->processLink( link );
                            }
                            else if ( res == actCopyLinkLocation )
                            {
                                const KPDFLinkBrowse * browseLink = static_cast< const KPDFLinkBrowse * >( link );
                                QClipboard *cb = QApplication::clipboard();
                                cb->setText( browseLink->url(), QClipboard::Clipboard );
                                if ( cb->supportsSelection() )
                                    cb->setText( browseLink->url(), QClipboard::Selection );
                            }
                        }
                    }
                    else
                    {
                        // a link can move us to another page or even to another document, there's no point in trying to
                        //  process the click on the image once we have processes the click on the link
                        rect = pageItem->page()->getObjectRect( ObjectRect::Image, nX, nY );
                        if ( rect )
                        {
                            // handle right click over a image
                        }
                        else
                        {
                            // right click (if not within 5 px of the press point, the mode
                            // had been already changed to 'Selection' instead of 'Normal')
                            emit rightClick( pageItem->page(), e->globalPos() );
                        }
                    }
                }
                else
                {
                    // right click (if not within 5 px of the press point, the mode
                    // had been already changed to 'Selection' instead of 'Normal')
                    emit rightClick( pageItem ? pageItem->page() : 0, e->globalPos() );
                }
            }
            }break;

        case MouseZoom:
            // if a selection rect has been defined, zoom into it
            if ( leftButton && d->mouseSelecting && d->rubberBand )
            {
                const QRectF & selRect = d->rubberBand->geometry();
                if ( selRect.width() <= 8 && selRect.height() <= 8 )
                {
                    selectionClear();
                    break;
                }

                const QRect & visibleRect = viewport()->rect();
                const QRectF & sceneRect = scene()->sceneRect();
                // find out new zoom ratio and normalized view center (relative to the contentsRect)
                double zoom = qMin( (double)visibleRect.width() / selRect.width(), (double)visibleRect.height() / selRect.height() );
                double nX = (selRect.left() + selRect.right()) / (2.0 * sceneRect.width());
                double nY = (selRect.top() + selRect.bottom()) / (2.0 * sceneRect.height());

                // zoom up to 400%
                if ( d->zoomFactor <= 4.0 || zoom <= 1.0 )
                {
                    d->zoomFactor *= zoom;
                    viewport()->setUpdatesEnabled( false );
                    updateZoom( ZoomRefreshCurrent );
                    viewport()->setUpdatesEnabled( true );
                }

                // recenter view and update the viewport
                centerOn( nX * sceneRect.width(), nY * sceneRect.height() );
//                update();

                // hide message box and delete overlay window
                selectionClear();
            }
            break;

        case MouseSelect:{
            // if mouse is released and selection is null this is a rightClick
            // FIXME!!!!
/*
            if ( rightButton && !d->mouseSelecting )
            {
                PageViewItem * pageItem = pickItemOnPoint( e->x(), e->y() );
                emit rightClick( pageItem ? pageItem->page() : 0, e->globalPos() );
                break;
            }
*/
            // if a selection is defined, display a popup
            if ( !d->rubberBand || !d->mouseSelecting )
                break;

            const QRectF & selectionRect = d->rubberBand->geometry();
            d->mouseSelecting = false;
            if ( selectionRect.width() <= 8 && selectionRect.height() <= 8 )
            {
                selectionClear();
/*
                if ( d->aPrevAction )
                {
                    d->aPrevAction->trigger();
                    d->aPrevAction = 0;
                }
*/
                break;
            }

            // if we support text generation 
            QString selectedText;
            if (d->document->supportsSearching())
            {
		// grab text in selection by extracting it from all intersected pages
		RegularAreaRect * rects=new RegularAreaRect;
		const KPDFPage * kpdfPage=0;
		QVector< PageViewItem * >::iterator iIt = d->items.begin(), iEnd = d->items.end();
		for ( ; iIt != iEnd; ++iIt )
		{
		    PageViewItem * item = *iIt;
		    const QRectF & itemRect = item->geometry();
		    if ( selectionRect.intersects( itemRect ) )
		    {
			// request the textpage if there isn't one
			kpdfPage= item->page();
			kWarning() << "checking if page " << item->pageNumber() << " has text " << kpdfPage->hasSearchPage() << endl;
			if ( !kpdfPage->hasSearchPage() )
			    d->document->requestTextPage( kpdfPage->number() );
			// grab text in the rect that intersects itemRect
			QRect relativeRect = selectionRect.intersect( itemRect ).toRect();
			relativeRect.translate( -itemRect.left(), -itemRect.top() );
			rects->append(new NormalizedRect( relativeRect, item->width(), item->height() ));
		    }
		}
		if (kpdfPage) selectedText = kpdfPage->getText( rects );
            }

            // popup that ask to copy:text and copy/save:image
            KMenu menu( this );
            QAction *textToClipboard = 0, *speakText = 0, *imageToClipboard = 0, *imageToFile = 0;
            if ( d->document->supportsSearching() && !selectedText.isEmpty() )
            {
                menu.addTitle( i18np( "Text (1 character)", "Text (%n characters)", selectedText.length() ) );
                textToClipboard = menu.addAction( SmallIconSet("editcopy"), i18n( "Copy to Clipboard" ) );
                if ( !d->document->isAllowed( KPDFDocument::AllowCopy ) )
                {
                    textToClipboard->setEnabled( false );
                    textToClipboard->setText( i18n("Copy forbidden by DRM") );
                }
                if ( KpdfSettings::useKTTSD() )
                    speakText = menu.addAction( SmallIconSet("kttsd"), i18n( "Speak Text" ) );
            }
            menu.addTitle( i18n( "Image (%1 by %2 pixels)", selectionRect.width(), selectionRect.height() ) );
            imageToClipboard = menu.addAction( QIcon(SmallIcon("image")), i18n( "Copy to Clipboard" ) );
            imageToFile = menu.addAction( QIcon(SmallIcon("filesave")), i18n( "Save to File..." ) );
            QAction *choice = menu.exec( e->globalPos() );
            // clear widget selection and invalidate rect
            selectionClear();
            // check if the user really selected an action
            if ( choice )
            {
            // IMAGE operation choosen
            if ( choice == imageToClipboard || choice == imageToFile )
            {
                // renders page into a pixmap
//kDebug() << "SELECTION: " << selectionRect << endl;
/*
                QPixmap copyPix( selectionRect.width(), selectionRect.height() );
                QPainter copyPainter( &copyPix );
                copyPainter.translate( -selectionRect.left(), -selectionRect.top() );
                drawDocumentOnPainter( selectionRect, &copyPainter );
*/
                QPixmap copyPix( selectionRect.toRect().size() );
                QPainter copyPainter( &copyPix );
                scene()->render( &copyPainter, QRectF(), selectionRect.toRect() );
                copyPainter.end();

                if ( choice == imageToClipboard )
                {
                    // [2] copy pixmap to clipboard
                    QClipboard *cb = QApplication::clipboard();
                    cb->setPixmap( copyPix, QClipboard::Clipboard );
                    if ( cb->supportsSelection() )
                        cb->setPixmap( copyPix, QClipboard::Selection );
                    d->messageWindow->display( i18n( "Image [%1x%2] copied to clipboard.", copyPix.width(), copyPix.height() ) );
                }
                else if ( choice == imageToFile )
                {
                    // [3] save pixmap to file
                    QString fileName = KFileDialog::getSaveFileName( KUrl(), "image/png image/jpeg", this );
                    if ( fileName.isEmpty() )
                        d->messageWindow->display( i18n( "File not saved." ), PageViewMessage::Warning );
                    else
                    {
                        KMimeType::Ptr mime = KMimeType::findByURL( fileName );
                        QString type;
                        if ( !mime )
                            type = "PNG";
                        else
                            type = mime->name().section( '/', -1 ).toUpper();
                        copyPix.save( fileName, qPrintable( type ) );
                        d->messageWindow->display( i18n( "Image [%1x%2] saved to %3 file.", copyPix.width(), copyPix.height(), type ) );
                    }
                }
            }
            // TEXT operation choosen
            else
            {
                if ( choice == textToClipboard )
                {
                    // [1] copy text to clipboard
                    QClipboard *cb = QApplication::clipboard();
                    cb->setText( selectedText, QClipboard::Clipboard );
                    if ( cb->supportsSelection() )
                        cb->setText( selectedText, QClipboard::Selection );
                }
                else if ( choice == speakText )
                {
                    // [2] speech selection using KTTSD
                    // Albert says is this ever necessary?
                    // we already attached on Part constructor
                    // If KTTSD not running, start it.
                    QDBusReply<bool> reply = QDBus::sessionBus().interface()->isServiceRegistered("org.kde.kttsd");
                    bool kttsdactive = false;
                    if ( reply.isValid() )
                        kttsdactive = reply.value();
                    if ( !kttsdactive )
                    {
                        QString error;
                        if (KToolInvocation::startServiceByDesktopName("kttsd", QStringList(), &error))
                        {
                            d->messageWindow->display( i18n("Starting KTTSD Failed: %1", error) );
                        }
                        else
                        {
                            kttsdactive = true;
                        }
                    }
                    if ( kttsdactive )
                    {
                        // creating the connection to the kspeech interface
                        QDBusInterface kspeech("org.kde.kttsd", "/KSpeech", "org.kde.KSpeech");
                        kspeech.call("setApplicationName", "okular");
                        kspeech.call("say", selectedText, 0);
                    }
                }
            }
            }

/*
            // restore previous action if came from it using right button
            if ( d->aPrevAction )
            {
                d->aPrevAction->trigger();
                d->aPrevAction = 0;
            }
*/
            }break;
            case MouseTextSelect:
                setCursor( Qt::ArrowCursor );
                // TODO: clear the selections (aka, notify the document to set null selections to the pages)
                if ( d->mouseTextSelecting )
                {
                    d->mouseTextSelecting = false;
/*
                    delete d->mouseTextSelectionInfo;
                    d->mouseTextSelectionInfo=0;
*/
//                    textSelectionClear();
                }
                else if ( !d->mousePressPos.isNull() )
                {
                    QPoint mapped = e->pos() + mapToScene( 0, 0 ).toPoint();
                    PageViewItem * pageItem = pickItemOnPoint( mapped.x(), mapped.y() );
                    const KPDFPage * page = pageItem ? pageItem->page() : 0;
                    if ( page )
                    {
                    }
                }
            break;
    }

    // reset mouse press / 'drag start' position
    d->mousePressPos = QPoint();

//    update();
}
}

void PageView::wheelEvent( QWheelEvent *e )
{
if (d->document->handleEvent( e ) )
{
    // don't perform any mouse action when viewport is autoscrolling
    if ( d->viewportMoveActive )
        return;

    int delta = e->delta(),
        vScroll = verticalScrollBar()->value();
    e->accept();
    if ( (e->modifiers() & Qt::ControlModifier) == Qt::ControlModifier ) {
        if ( e->delta() < 0 )
            slotZoomOut();
        else
            slotZoomIn();
    }
    else if ( delta <= -120 && !KpdfSettings::viewContinuous() && vScroll == verticalScrollBar()->maximum() )
    {
        // go to next page
        if ( d->document->currentPage() < d->items.count() - 1 )
        {
            // more optimized than document->setNextPage and then move view to top
            DocumentViewport newViewport = d->document->viewport();
            newViewport.pageNumber += d->document->currentPage() ? viewColumns() : 1;
            if ( newViewport.pageNumber >= (int)d->items.count() )
                newViewport.pageNumber = d->items.count() - 1;
            newViewport.rePos.enabled = true;
            newViewport.rePos.normalizedY = 0.0;
            d->document->setViewport( newViewport );
        }
    }
    else if ( delta >= 120 && !KpdfSettings::viewContinuous() && vScroll == verticalScrollBar()->minimum() )
    {
        // go to prev page
        if ( d->document->currentPage() > 0 )
        {
            // more optimized than document->setPrevPage and then move view to bottom
            DocumentViewport newViewport = d->document->viewport();
            newViewport.pageNumber -= viewColumns();
            if ( newViewport.pageNumber < 0 )
                newViewport.pageNumber = 0;
            newViewport.rePos.enabled = true;
            newViewport.rePos.normalizedY = 1.0;
            d->document->setViewport( newViewport );
        }
    }
    else
        QGraphicsView::wheelEvent( e );

    updateCursor( e->pos() + mapToScene( 0, 0 ).toPoint() );
}
}

void PageView::dragEnterEvent( QDragEnterEvent * ev )
{
if (d->document->handleEvent( ev ) )
{
    ev->accept();
}
}

void PageView::dragMoveEvent( QDragMoveEvent * ev )
{
    if ( !d->document->handleEvent( ev ) )
        return;

    ev->accept();
}

void PageView::dropEvent( QDropEvent * ev )
{
if (d->document->handleEvent( ev ) )
{
    if (  KUrl::List::canDecode(  ev->mimeData() ) )
        emit urlDropped( KUrl::List::fromMimeData( ev->mimeData() ).first() );
}
}
//END widget events

/*
void PageView::drawDocumentOnPainter( const QRect & contentsRect, QPainter * p )
{
    // when checking if an Item is contained in contentsRect, instead of
    // growing PageViewItems rects (for keeping outline into account), we
    // grow the contentsRect
    QRect checkRect = contentsRect;
    checkRect.adjust( -3, -3, 1, 1 );

    // create a region from wich we'll subtract painted rects
    QRegion remainingArea( contentsRect );

    // iterate over all items painting the ones intersecting contentsRect
    QVector< PageViewItem * >::iterator iIt = d->items.begin(), iEnd = d->items.end();
    for ( ; iIt != iEnd; ++iIt )
    {
        // check if a piece of the page intersects the contents rect
        if ( !(*iIt)->geometry().intersects( checkRect ) )
            continue;

        // get item and item's outline geometries
        PageViewItem * item = *iIt;
        QRect itemGeometry = item->geometry().toRect(),
              outlineGeometry = itemGeometry;
        outlineGeometry.adjust( -1, -1, 3, 3 );

        // move the painter to the top-left corner of the page
        p->save();
        p->translate( itemGeometry.left(), itemGeometry.top() );

        // draw the page outline (black border and 2px bottom-right shadow)
        if ( !itemGeometry.contains( contentsRect ) )
        {
            int itemWidth = itemGeometry.width(),
                itemHeight = itemGeometry.height();
            // draw simple outline
            p->setPen( Qt::black );
            p->drawRect( -1, -1, itemWidth + 2, itemHeight + 2 );
            // draw bottom/right gradient
            int levels = 2;
            int r = QColor(Qt::gray).red() / (levels + 2),
                g = QColor(Qt::gray).green() / (levels + 2),
                b = QColor(Qt::gray).blue() / (levels + 2);
            for ( int i = 0; i < levels; i++ )
            {
                p->setPen( QColor( r * (i+2), g * (i+2), b * (i+2) ) );
                p->drawLine( i, i + itemHeight + 1, i + itemWidth + 1, i + itemHeight + 1 );
                p->drawLine( i + itemWidth + 1, i, i + itemWidth + 1, i + itemHeight );
                p->setPen( Qt::gray );
                p->drawLine( -1, i + itemHeight + 1, i - 1, i + itemHeight + 1 );
                p->drawLine( i + itemWidth + 1, -1, i + itemWidth + 1, i - 1 );
            }
        }

        // draw the page using the PagePainter whith all flags active
        if ( contentsRect.intersects( itemGeometry ) )
        {
            QRect pixmapRect = contentsRect.intersect( itemGeometry );
            pixmapRect.translate( -itemGeometry.left(), -itemGeometry.top() );
            PagePainter::paintPageOnPainter( p, item->page(), PAGEVIEW_ID, pageflags,
                itemGeometry.width(), itemGeometry.height(), pixmapRect );
        }

        // remove painted area from 'remainingArea' and restore painter
        remainingArea -= outlineGeometry.intersect( contentsRect );
        p->restore();
    }

    // fill with background color the unpainted area
    QVector<QRect> backRects = remainingArea.rects();
    int backRectsNumber = backRects.count();
    QColor backColor = / * d->items.isEmpty() ? Qt::lightGray : * / Qt::gray;
    for ( int jr = 0; jr < backRectsNumber; jr++ )
        p->fillRect( backRects[ jr ], backColor );
}
*/

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
        zoom = qMin( scaleW, scaleH );
        item->setWHZ( (int)(zoom * width), (int)(zoom * height), zoom );
        d->zoomFactor = zoom;
    }
#ifndef NDEBUG
    else
        kDebug() << "calling updateItemSize with unrecognized d->zoomMode!" << endl;
#endif
}

PageViewItem * PageView::pickItemOnPoint( int x, int y )
{
    PageViewItem * item = 0;
    QLinkedList< PageViewItem * >::iterator iIt = d->visibleItems.begin(), iEnd = d->visibleItems.end();
    for ( ; iIt != iEnd; ++iIt )
    {
        PageViewItem * i = *iIt;
        const QRectF & r = i->geometry();
        if ( x < r.right() && x > r.left() && y < r.bottom() )
        {
            if ( y > r.top() )
                item = i;
            break;
        }
    }
    return item;
}

/*
void PageView::textSelection( QList<QRect> * area, const QColor & color )
{
    setCursor( Qt::IBeamCursor );
    QList<QRect> toUpdate;
    if ( d->mouseTextSelectionRect && d->mouseTextSelectionRect )
    {
      toUpdate+=*(d->mouseTextSelectionRect);
      delete d->mouseTextSelectionRect;
      d->mouseTextSelectionRect=0;
    }
    
    if (area)
      toUpdate+=*(area);
    
    d->mouseTextSelecting = true;
    d->mouseTextSelectionRect = area;
    d->mouseTextSelectionColor = color;
    // ensures page doesn't scroll
    if ( d->autoScrollTimer )
    {
        d->scrollIncrement = 0;
        d->autoScrollTimer->stop();
    }
    QList<QRect>::Iterator it=toUpdate.begin(), end=toUpdate.end();
    QRect r;
    if ( it != end )
        r = (*it).adjusted( 0, 0, 1, 1 );
    for (;it!=end;++it)
    {
        r |= (*it).adjusted( 0, 0, 1, 1 );
    }
    if ( !r.isNull() )
        scene()->update( r );
    d->mouseTextSelectionPainted=true;
}
*/
    
void PageView::textSelectionClear()
{
  // nothing no clear
//  if ( !d->mouseTextSelectionRect ) return;

//  setCursor( Qt::ArrowCursor  );
  // TODO: clear the selections (aka, notify the document to set null selections to the pages)
/*
    PageViewItem * pageItem = pickItemOnPoint( pos.x(), pos.y() );
    if ( pageItem )
    {
        d->document->setPageTextSelection( pageItem->pageNumber(), 0, QColor() );
    }
*/
    if ( !d->pagesWithTextSelection.isEmpty() )
    {
        QSet< int >::ConstIterator it = d->pagesWithTextSelection.constBegin(), itEnd = d->pagesWithTextSelection.constEnd();
        for ( ; it != itEnd; ++it )
            d->document->setPageTextSelection( *it, 0, QColor() );
        d->pagesWithTextSelection.clear();
    }
/*
  QList<QRect>::iterator it=d->mouseTextSelectionRect->begin(),
    end=d->mouseTextSelectionRect->end();
 for (;it!=end;++it)
  {
    scene()->update( *it );
  }
*/
/*
  delete d->mouseTextSelectionRect;
  d->mouseTextSelectionRect=0;
*/
}

void PageView::selectionStart( const QPoint & pos, const QColor & color, bool /*aboveAll*/ )
{
    selectionClear();
    d->mouseSelecting = true;
    d->rubberBand = new RubberBandItem( color );
    scene()->addItem( d->rubberBand );
    d->rubberBand->setPos( mapToScene( 0, 0 ) + pos );
    d->rubberBand->resize( 1, 1 );
    // ensures page doesn't scroll
    if ( d->autoScrollTimer )
    {
        d->scrollIncrement = 0;
        d->autoScrollTimer->stop();
    }
}

void PageView::textSelectionForItem( PageViewItem * item, const QPoint & startPoint, const QPoint & endPoint )
{
    QColor selColor = palette().color( QPalette::Active, QPalette::Highlight );
    const QRectF & geometry = item->geometry();
    NormalizedPoint startCursor( 0.0, 0.0 );
    if ( !startPoint.isNull() )
    {
        startCursor = NormalizedPoint( startPoint.x(), startPoint.y(), geometry.width(), geometry.height() );
    }
    NormalizedPoint endCursor( 1.0, 1.0 );
    if ( !endPoint.isNull() )
    {
        endCursor = NormalizedPoint( endPoint.x(), endPoint.y(), geometry.width(), geometry.height() );
    }
    ::TextSelection mouseTextSelectionInfo( startCursor, endCursor );

    const KPDFPage * kpdfPage = item->page();

    if ( !kpdfPage->hasSearchPage() )
        d->document->requestTextPage( kpdfPage->number() );

    RegularAreaRect * selectionArea = kpdfPage->getTextArea( &mouseTextSelectionInfo );
    if ( selectionArea && ( !selectionArea->isEmpty() ) )
    {
        kDebug() << "text areas (" << kpdfPage->number() << "): " << selectionArea->count() << endl;
        d->document->setPageTextSelection( kpdfPage->number(), selectionArea, selColor );
    }
    qDeleteAll(*selectionArea);
    delete selectionArea;
}

void PageView::selectionClear()
{
    d->mouseSelecting = false;
    if ( d->rubberBand )
    {
        scene()->removeItem( d->rubberBand );
        const QRectF & updatedRect = d->rubberBand->geometry();
        scene()->update( updatedRect );
        delete d->rubberBand;
        d->rubberBand = 0;
    }
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
            // kdelibs4 sometimes adds accelerators to actions' text directly :(
            z.remove ('&');
            z.remove ('%');
            newFactor = KGlobal::locale()->readNumber( z ) / 100.0;
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
        // store zoom settings
        KpdfSettings::setZoomMode( newZoomMode );
        KpdfSettings::setZoomFactor( newFactor );
        KpdfSettings::writeConfig();
    }
}

void PageView::updateZoomText()
{
    // use current page zoom as zoomFactor if in ZoomFit/* mode
    if ( d->zoomMode != ZoomFixed && d->items.count() > 0 )
        d->zoomFactor = d->items[ qMax( 0, (int)d->document->currentPage() ) ]->zoomFactor();
    float newFactor = d->zoomFactor;
    d->aZoom->removeAllActions();

    // add items that describe fit actions
    QStringList translated;
    translated << i18n("Fit Width") << i18n("Fit Page") /*<< i18n("Fit Text")*/;

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
        const QRectF & geom = pageItem->geometry();
        double nX = (p.x() - geom.left()) / geom.width(),
               nY = (p.y() - geom.top()) / geom.height();

        // if over a ObjectRect (of type Link) change cursor to hand
        if ( d->mouseMode == MouseTextSelect )
            setCursor( Qt::IBeamCursor );
        else
        {
            d->mouseOnRect = pageItem->page()->getObjectRect( ObjectRect::Link, nX, nY );
            if ( d->mouseOnRect )
                setCursor( Qt::PointingHandCursor );
            else
                setCursor( Qt::ArrowCursor );
        }
    }
    else
    {
        // if there's no page over the cursor and we were showing the pointingHandCursor
        // go back to the normal one
        d->mouseOnRect = false;
        setCursor( Qt::ArrowCursor );
    }
}

int PageView::viewColumns()
{
    int nr=KpdfSettings::renderMode();
    if (nr<2)
	return nr+1;
    return KpdfSettings::viewColumns();
}

int PageView::viewRows()
{
    if (KpdfSettings::renderMode()<2)
	return 1;
    return KpdfSettings::viewRows();
}

//BEGIN private SLOTS
void PageView::slotRelayoutPages()
// called by: notifySetup, viewportResizeEvent, slotRenderMode, slotContinuousToggled, updateZoom
{
    // first reset the scene rect
    setSceneRect( QRectF() );

    // set an empty container if we have no pages
    int pageCount = d->items.count();
    if ( pageCount < 1 )
    {
        setSceneRect( 0, 0, 0, 0 );
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
    QVector< PageViewItem * >::iterator iIt, iEnd = d->items.end();
    QRect viewportRect = viewport()->rect().translated( mapToScene( 0, 0 ).toPoint() );
    int viewportWidth = viewportRect.width(),
        viewportHeight = viewportRect.height(),
        fullWidth = 0,
        fullHeight = 0;

    // handle the 'center first page in row' stuff
    int nCols = viewColumns();
    bool centerFirstPage = KpdfSettings::centerFirstPageInRow() && nCols > 1;

    // set all items geometry and resize contents. handle 'continuous' and 'single' modes separately
    if ( KpdfSettings::viewContinuous() )
    {
        // handle the 'centering on first row' stuff
        if ( centerFirstPage )
            pageCount += nCols - 1;
        // Here we find out column's width and row's height to compute a table
        // so we can place widgets 'centered in virtual cells'.
	int nRows;

// 	if ( KpdfSettings::renderMode() < 2 )
        	nRows = (int)ceil( (float)pageCount / (float)nCols );
// 		nRows=(int)ceil( (float)pageCount / (float) KpdfSettings::viewRows() );
// 	else
// 		nRows = KpdfSettings::viewRows();

        int * colWidth = new int[ nCols ],
            * rowHeight = new int[ nRows ],
            cIdx = 0,
            rIdx = 0;
        for ( int i = 0; i < nCols; i++ )
            colWidth[ i ] = viewportWidth / nCols;
        for ( int i = 0; i < nRows; i++ )
            rowHeight[ i ] = 0;
        // handle the 'centering on first row' stuff
        if ( centerFirstPage )
            pageCount -= nCols - 1;

        // 1) find the maximum columns width and rows height for a grid in
        // which each page must well-fit inside a cell
        for ( iIt = d->items.begin(); iIt != iEnd; ++iIt )
        {
            PageViewItem * item = *iIt;
            // update internal page size (leaving a little margin in case of Fit* modes)
            updateItemSize( item, colWidth[ cIdx ] - 6, viewportHeight - 12 );
            // find row's maximum height and column's max width
            if ( item->width() + 6 > colWidth[ cIdx ] )
                colWidth[ cIdx ] = item->width() + 6;
            if ( item->height() + 12 > rowHeight[ rIdx ] )
                rowHeight[ rIdx ] = item->height() + 12;
            // handle the 'centering on first row' stuff
            if ( centerFirstPage && !item->pageNumber() )
                cIdx += nCols - 1;
            // update col/row indices
            if ( ++cIdx == nCols )
            {
                cIdx = 0;
                rIdx++;
            }
        }

        // 2) compute full size
        for ( int i = 0; i < nCols; i++ )
            fullWidth += colWidth[ i ];
        for ( int i = 0; i < nRows; i++ )
            fullHeight += rowHeight[ i ];

        // 3) arrange widgets inside cells (and refine fullHeight if needed)
        int insertX = 0,
            insertY = fullHeight < viewportHeight ? ( viewportHeight - fullHeight ) / 2 : 0;
        cIdx = 0;
        rIdx = 0;
        for ( iIt = d->items.begin(); iIt != iEnd; ++iIt )
        {
            PageViewItem * item = *iIt;
            int cWidth = colWidth[ cIdx ],
                rHeight = rowHeight[ rIdx ];
            if ( centerFirstPage && !rIdx && !cIdx )
            {
                // handle the 'centering on first row' stuff
                item->moveTo( insertX + (fullWidth - item->width()) / 2,
                              insertY + (rHeight - item->height()) / 2 );
                cIdx += nCols - 1;
            }
            else
            {
                // center widget inside 'cells'
                item->moveTo( insertX + (cWidth - item->width()) / 2,
                              insertY + (rHeight - item->height()) / 2 );
            }
            // advance col/row index
            insertX += cWidth;
            if ( ++cIdx == nCols )
            {
                cIdx = 0;
                rIdx++;
                insertX = 0;
                insertY += rHeight;
            }
#ifdef PAGEVIEW_DEBUG
            kWarning() << "updating size for pageno " << item->pageNumber() << " to " << item->geometry() << endl;
#endif
        }

        delete [] colWidth;
        delete [] rowHeight;
    }
    else // viewContinuous is FALSE
    {
        PageViewItem * currentItem = d->items[ qMax( 0, (int)d->document->currentPage() ) ];
	
	int nRows=viewRows();

        // handle the 'centering on first row' stuff
        if ( centerFirstPage && d->document->currentPage() < 1 && KpdfSettings::renderMode() == 1 )
            nCols = 1, nRows=1;

        // setup varialbles for a M(row) x N(columns) grid
        int * colWidth = new int[ nCols ],
            cIdx = 0;

        int * rowHeight = new int[ nRows ],
            rIdx = 0;

        for ( int i = 0; i < nCols; i++ )
            colWidth[ i ] = viewportWidth / nCols;

        for ( int i = 0; i < nRows; i++ )
            rowHeight[ i ] = viewportHeight / nRows;

        // 1) find out maximum area extension for the pages
	bool wasCurrent = false;
        for ( iIt = d->items.begin(); iIt != iEnd; ++iIt )
        {
            PageViewItem * item = *iIt;
            if ( rIdx >= 0 && rIdx < nRows )
            {
            	if ( item == currentItem )
			wasCurrent=true;
		if ( wasCurrent && cIdx >= 0 && cIdx < nCols )
            	{
			// update internal page size (leaving a little margin in case of Fit* modes)
			updateItemSize( item, colWidth[ cIdx ] - 6, rowHeight[ rIdx ] - 12 );
			// find row's maximum height and column's max width
			if ( item->width() + 6 > colWidth[ cIdx ] )
			colWidth[ cIdx ] = item->width() + 6;
			if ( item->height() + 12 > rowHeight[ rIdx ] )
			rowHeight[ rIdx ] = item->height() + 12;
			cIdx++;
		}
		if( cIdx>=nCols )
		{
            		rIdx++;
            		cIdx=0;
            	}
            }
        }

        // 2) calc full size (fullHeight is alredy ok)
        for ( int i = 0; i < nCols; i++ )
            fullWidth += colWidth[ i ];

         for ( int i = 0; i < nRows; i++ )
            fullHeight += rowHeight[ i ];

        // 3) hide all widgets except the displayable ones and dispose those
        int insertX = 0;
	int insertY = 0;
        cIdx = 0;
	rIdx = 0;
	wasCurrent=false;
        for ( iIt = d->items.begin(); iIt != iEnd; ++iIt )
        {
            PageViewItem * item = *iIt;
            if ( rIdx >= 0 && rIdx < nRows )
            {
            	if ( item == currentItem )
			wasCurrent=true;
		if ( wasCurrent && cIdx >= 0 && cIdx < nCols )
		{
			// center widget inside 'cells'
			item->moveTo( insertX + (colWidth[ cIdx ] - item->width()) / 2,
				insertY + ( rowHeight[ rIdx ] - item->height() ) / 2 );
			// advance col index
			insertX += colWidth[ cIdx ];
			cIdx++;
		} else
			// pino: I used invalidate() instead of setGeometry() so
			// the geometry rect of the item is really invalidated
			//item->setGeometry( 0, 0, -1, -1 );
			item->invalidate();

		if( cIdx>=nCols)
		{
			insertY += rowHeight[ rIdx ];
            		rIdx++;
			insertX = 0;
            		cIdx=0;
            	}
            }
            else
		// pino: I used invalidate() instead of setGeometry() so
		// the geometry rect of the item is really invalidated
		//item->setGeometry( 0, 0, -1, -1 );
		item->invalidate();
        }

        delete [] colWidth;
	delete [] rowHeight;
    }

    // 3) reset dirty state
    d->dirtyLayout = false;

    // 4) update scrollview's contents size and recenter view
    bool wasUpdatesEnabled = updatesEnabled();
    if ( fullWidth != viewportWidth || fullHeight != viewportHeight )
    {
        // disable updates and resize the viewportContents
        if ( wasUpdatesEnabled )
            setUpdatesEnabled( false );
        setSceneRect( sceneRect() );
        // restore previous viewport if defined and updates enabled
        if ( wasUpdatesEnabled )
        {
            const DocumentViewport & vp = d->document->viewport();
            if ( vp.pageNumber >= 0 )
            {
                int prevX = viewportRect.left(),
                    prevY = viewportRect.top();
                const QRectF & geometry = d->items[ vp.pageNumber ]->geometry();
                double nX = vp.rePos.enabled ? vp.rePos.normalizedX : 0.5,
                       nY = vp.rePos.enabled ? vp.rePos.normalizedY : 0.0;
                centerOn( geometry.left() + ROUND( nX * geometry.width() ),
                        geometry.top() + ROUND( nY * geometry.height() ) );
                // centerOn() usually moves the viewport, that requests pixmaps too.
                // if that doesn't happen we have to request them by hand
                if ( prevX == viewportRect.left() && prevY == viewportRect.top() )
                    slotRequestVisiblePixmaps();
            }
            // or else go to center page
            else
                centerOn( fullWidth / 2, 0 );
            setUpdatesEnabled( true );
        }
    }

    // 5) update the whole viewport if updated enabled
    if ( wasUpdatesEnabled )
        update();
}

void PageView::slotRequestVisiblePixmaps( int value )
{
    // if requests are blocked (because raised by an unwanted event), exit
    if ( d->blockPixmapsRequest || d->viewportMoveActive ||
         d->mouseMidZooming )
        return;

    // precalc view limits for intersecting with page coords inside the lOOp
    bool isEvent = value != -1 && !d->blockViewport;
    QRect viewportRect = viewport()->rect().translated( mapToScene( 0, 0 ).toPoint() );

    // some variables used to determine the viewport
    int nearPageNumber = -1;
    double viewportCenterX = (viewportRect.left() + viewportRect.right()) / 2.0,
           viewportCenterY = (viewportRect.top() + viewportRect.bottom()) / 2.0,
           focusedX = 0.5,
           focusedY = 0.0,
           minDistance = -1.0;

    // iterate over all items
    d->visibleItems.clear();
    QLinkedList< PixmapRequest * > requestedPixmaps;
    QVector< VisiblePageRect * > visibleRects;
    QVector< PageViewItem * >::iterator iIt = d->items.begin(), iEnd = d->items.end();
    for ( ; iIt != iEnd; ++iIt )
    {
        PageViewItem * i = *iIt;
#ifdef PAGEVIEW_DEBUG
        kWarning() << "checking page " << i->pageNumber() << endl;
        kWarning() << "viewportRect is " << viewportRect << ", page item is " << i->geometry() << " intersect : " << viewportRect.intersects( i->geometry().toRect() ) << endl;
#endif
        // if the item doesn't intersect the viewport, skip it
        QRect intersectionRect = viewportRect.intersect( i->geometry().toRect() );
        if ( intersectionRect.isEmpty() )
            continue;

        // add the item to the 'visible list'
        d->visibleItems.push_back( i );
        VisiblePageRect * vItem = new VisiblePageRect( i->pageNumber(), NormalizedRect( intersectionRect.translated( -i->geometry().toRect().topLeft() ), i->width(), i->height() ) );
        visibleRects.push_back( vItem );
#ifdef PAGEVIEW_DEBUG
        kWarning() << "checking for pixmap for page " << i->pageNumber() <<  " = " << i->page()->hasPixmap( PAGEVIEW_ID, i->width(), i->height() ) << "\n";
#endif
        kWarning() << "checking for text for page " << i->pageNumber() <<  " = " << i->page()->hasSearchPage() << "\n";
        // if the item has not the right pixmap, add a request for it
        if ( !i->page()->hasPixmap( PAGEVIEW_ID, i->width(), i->height() ) )
        {
#ifdef PAGEVIEW_DEBUG
            kWarning() << "rerequesting visible pixmaps for page " << i->pageNumber() <<  " !\n";
#endif
            PixmapRequest * p = new PixmapRequest(
                    PAGEVIEW_ID, i->pageNumber(), i->width(), i->height(), PAGEVIEW_PRIO, true );
            requestedPixmaps.push_back( p );
        }

        // look for the item closest to viewport center and the relative
        // position between the item and the viewport center
        if ( isEvent )
        {
            const QRectF & geometry = i->geometry();
            // compute distance between item center and viewport center (slightly moved left)
            double distance = hypot( (geometry.left() + geometry.right()) / 2 - (viewportCenterX - 4),
                                     (geometry.top() + geometry.bottom()) / 2 - viewportCenterY );
            if ( distance >= minDistance && nearPageNumber != -1 )
                continue;
            nearPageNumber = i->pageNumber();
            minDistance = distance;
            if ( geometry.height() > 0 && geometry.width() > 0 )
            {
                focusedX = ( viewportCenterX - geometry.left() ) / geometry.width();
                focusedY = ( viewportCenterY - geometry.top() ) / geometry.height();
            }
        }
    }

    // if preloading is enabled, add the pages before and after in preloading
    if ( !d->visibleItems.isEmpty() &&
         KpdfSettings::memoryLevel() != KpdfSettings::EnumMemoryLevel::Low &&
         KpdfSettings::enableThreading() )
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
    {
        d->document->requestPixmaps( requestedPixmaps );
    }
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
    d->document->setVisiblePageRects( visibleRects, PAGEVIEW_ID );
}

void PageView::slotMoveViewport()
{
    // converge to viewportMoveDest in 1 second
    int diffTime = d->viewportMoveTime.elapsed();
    if ( diffTime >= 667 || !d->viewportMoveActive )
    {
        centerOn( d->viewportMoveDest.x(), d->viewportMoveDest.y() );
        d->viewportMoveTimer->stop();
        d->viewportMoveActive = false;
        slotRequestVisiblePixmaps();
        verticalScrollBar()->setEnabled( true );
        horizontalScrollBar()->setEnabled( true );
        return;
    }

//    const QRect & viewportRect = viewport()->rect();
    QRect viewportRect = viewport()->rect().translated( mapToScene( 0, 0 ).toPoint() );
    // move the viewport smoothly (kmplot: p(x)=1+0.47*(x-1)^3-0.25*(x-1)^4)
   // FIXME!!!!!
    float convergeSpeed = (float)diffTime / 667.0,
          x = ((float)viewportRect.width() / 2.0) + viewportRect.left(),
          y = ((float)viewportRect.height() / 2.0) + viewportRect.top(),
          diffX = (float)d->viewportMoveDest.x() - x,
          diffY = (float)d->viewportMoveDest.y() - y;
    convergeSpeed *= convergeSpeed * (1.4 - convergeSpeed);
    centerOn( (int)(x + diffX * convergeSpeed),
            (int)(y + diffY * convergeSpeed ) );
}

void PageView::slotAutoScoll()
{
    // the first time create the timer
    if ( !d->autoScrollTimer )
    {
        d->autoScrollTimer = new QTimer( this );
        d->autoScrollTimer->setSingleShot( true );
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
    d->autoScrollTimer->start( scrollDelay[ index ] );
    scrollContentsBy( 0, d->scrollIncrement > 0 ? scrollOffset[ index ] : -scrollOffset[ index ] );
}

void PageView::slotStopFindAhead()
{
    d->typeAheadActive = false;
    d->typeAheadString = "";
    d->messageWindow->display( i18n("Find stopped."), PageViewMessage::Find, 1000 );
    // (4/4) it is needed to grab the keyboard becase people may have Space assigned
    // to a accel and without grabbing the keyboard you can not vim-search for space
    // because it activates the accel
    releaseKeyboard();
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

void PageView::slotRenderMode( int nr )
{
    uint newColumns;
    if (nr<2)
	newColumns = nr+1;
    else
	newColumns = KpdfSettings::viewColumns();

    if ( KpdfSettings::renderMode() != nr )
    {
        KpdfSettings::setRenderMode( nr );
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
    // hide the messageWindow
    d->messageWindow->hide();
    // reshow the annotator toolbar if hiding was forced
    if ( d->aToggleAnnotator->isChecked() )
        slotToggleAnnotator( true );
}

void PageView::slotSetMouseZoom()
{
    d->mouseMode = MouseZoom;
    // change the text in messageWindow (and show it if hidden)
    d->messageWindow->display( i18n( "Select zooming area. Right-click to zoom out." ), PageViewMessage::Info, -1 );
    // force hiding of annotator toolbar
    if ( d->annotator )
        d->annotator->setEnabled( false );
}

void PageView::slotSetMouseSelect()
{
    d->mouseMode = MouseSelect;
    // change the text in messageWindow (and show it if hidden)
    d->messageWindow->display( i18n( "Draw a rectangle around the text/graphics to copy." ), PageViewMessage::Info, -1 );
    // force hiding of annotator toolbar
    if ( d->annotator )
        d->annotator->setEnabled( false );
}

void PageView::slotSetMouseTextSelect()
{
    d->mouseMode = MouseTextSelect;
    // change the text in messageWindow (and show it if hidden)
    d->messageWindow->display( i18n( "Select text." ), PageViewMessage::Info, -1 );
    // force hiding of annotator toolbar
    if ( d->annotator )
        d->annotator->setEnabled( false );
}

void PageView::slotToggleAnnotator( bool on )
{
    // only use annotator in normal mouse mode
    if ( on && d->mouseMode != MouseNormal )
        return;

    // create the annotator object if not present
    if ( !d->annotator )
    {
        d->annotator = new PageViewAnnotator( this, d->document );
        QVector< PageViewItem * >::iterator dIt = d->items.begin(), dEnd = d->items.end();
        for ( ; dIt != dEnd; ++dIt )
            (*dIt)->setAnnotator( d->annotator );
    }

    // initialize/reset annotator (and show/hide toolbar)
    d->annotator->setEnabled( on );
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
