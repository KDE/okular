/***************************************************************************
 *   Copyright (C) 2004-2006 by Albert Astals Cid <tsdgeos@terra.es>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "thumbnaillist.h"

// qt/kde includes
#include <qevent.h>
#include <qtimer.h>
#include <qpainter.h>
#include <qscrollbar.h>
#include <qsizepolicy.h>
#include <klocale.h>
#include <kurl.h>
#include <kaction.h>
#include <kdialog.h>
#include <kiconloader.h>
#include <kactioncollection.h>
#include <kicon.h>

// local includes
#include "pagepainter.h"
#include "core/area.h"
#include "core/document.h"
#include "core/generator.h"
#include "core/page.h"
#include "settings.h"

// ThumbnailWidget represents a single thumbnail in the ThumbnailList
class ThumbnailWidget : public QWidget
{
    public:
        ThumbnailWidget( QWidget * parent, const Okular::Document * document, const Okular::Page * page, ThumbnailList * tl );

        // set internal parameters to fit the page in the given width
        void resizeFitWidth( int width );
        // set thumbnail's selected state
        void setSelected( bool selected );
        // set the visible rect of the current page
        void setVisibleRect( const Okular::NormalizedRect & rect );

        // query methods
        int heightHint() const { return m_pixmapHeight + m_labelHeight + m_margin; }
        int pixmapWidth() const { return m_pixmapWidth; }
        int pixmapHeight() const { return m_pixmapHeight; }
        int pageNumber() const { return m_page->number(); }
        const Okular::Page * page() const { return m_page; }
        QSize sizeHint() const;

    protected:
        void mousePressEvent( QMouseEvent * e );
        void mouseReleaseEvent( QMouseEvent * e );
        void mouseMoveEvent( QMouseEvent * e );
        void wheelEvent( QWheelEvent * e );
        void contextMenuEvent( QContextMenuEvent * e );
        void paintEvent(QPaintEvent *);

    private:
        // the margin around the widget
        static int const m_margin = 16;

        // used to access 'forwardClick( .. )' and 'getBookmarkOverlay()'
        ThumbnailList * m_tl;
        const Okular::Document * m_document;
        const Okular::Page * m_page;
        bool m_selected;
        int m_pixmapWidth, m_pixmapHeight;
        int m_labelHeight, m_labelNumber;
        Okular::NormalizedRect m_visibleRect;
        QPoint mouseGrabPos;
};


/** ThumbnailList implementation **/

ThumbnailList::ThumbnailList( QWidget *parent, Okular::Document *document )
    : QScrollArea( parent ),
    m_document( document ), m_selected( 0 ), m_delayTimer( 0 ), m_bookmarkOverlay( 0 )
{
    setObjectName( "okular::Thumbnails" );
    // set scrollbars
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );

    setAttribute( Qt::WA_StaticContents );

    setAcceptDrops( true );

    QPalette pal = palette();
    // set contents background to the 'base' color
    QPalette viewportPal = viewport()->palette();
    viewportPal.setColor( viewport()->backgroundRole(), pal.color( QPalette::Base ) );
    viewport()->setPalette( viewportPal );

    m_pagesWidget = new QWidget();
    setWidget( m_pagesWidget );
    // widget setup: can be focused by tab and mouse click (not wheel)
    m_pagesWidget->setFocusPolicy( Qt::StrongFocus );
    m_pagesWidget->show();
    QPalette widgetPal = m_pagesWidget->palette();
    widgetPal.setColor( m_pagesWidget->backgroundRole(), pal.color( QPalette::Base ) );
    m_pagesWidget->setPalette( widgetPal );

    setFrameStyle( StyledPanel | Raised );

    connect( verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotRequestVisiblePixmaps(int)) );
}

ThumbnailList::~ThumbnailList()
{
    m_document->removeObserver( this );
    delete m_bookmarkOverlay;
}

//BEGIN DocumentObserver inherited methods
void ThumbnailList::notifySetup( const QVector< Okular::Page * > & pages, int setupFlags )
{
    // if there was a widget selected, save its pagenumber to restore
    // its selection (if available in the new set of pages)
    int prevPage = -1;
    if ( !( setupFlags & Okular::DocumentObserver::DocumentChanged ) && m_selected )
    {
        prevPage = m_selected->page()->number();
    }

    // delete all the Thumbnails
    QVector<ThumbnailWidget *>::const_iterator tIt = m_thumbnails.begin(), tEnd = m_thumbnails.end();
    for ( ; tIt != tEnd; ++tIt )
        delete *tIt;
    m_thumbnails.clear();
    m_visibleThumbnails.clear();
    m_selected = 0;

    if ( pages.count() < 1 )
    {
        m_pagesWidget->resize( 0, 0 );
        return;
    }

    // show pages containing hilighted text or bookmarked ones
    //RESTORE THIS int flags = Okular::Settings::filterBookmarks() ? Okular::Page::Bookmark : Okular::Page::Highlight;

    // if no page matches filter rule, then display all pages
    QVector< Okular::Page * >::const_iterator pIt = pages.begin(), pEnd = pages.end();
    bool skipCheck = true;
    for ( ; pIt != pEnd ; ++pIt )
        //if ( (*pIt)->attributes() & flags )
        if ( (*pIt)->hasHighlights( SW_SEARCH_ID ) )
            skipCheck = false;

    // generate Thumbnails for the given set of pages
    int width = viewport()->width();
    int height = 0;
    for ( pIt = pages.begin(); pIt != pEnd ; ++pIt )
        //if ( skipCheck || (*pIt)->attributes() & flags )
        if ( skipCheck || (*pIt)->hasHighlights( SW_SEARCH_ID ) )
        {
            ThumbnailWidget * t = new ThumbnailWidget( widget(), m_document, *pIt, this );
            t->setFocusProxy( this );
            t->move(0, height);
            // add to the internal queue
            m_thumbnails.push_back( t );
            // update total height (asking widget its own height)
            t->resizeFitWidth( width );
            // restoring the previous selected page, if any
            if ( (*pIt)->number() == prevPage )
            {
                m_selected = t;
                m_selected->setSelected( true );
            }
            t->show();
            height += t->height() + KDialog::spacingHint();
        }

    // update scrollview's contents size (sets scrollbars limits)
    height -= KDialog::spacingHint();
    m_pagesWidget->resize( width, height );

    // request for thumbnail generation
    delayedRequestVisiblePixmaps( 200 );
}

void ThumbnailList::notifyViewportChanged( bool /*smoothMove*/ )
{
    // skip notifies for the current page (already selected)
    int newPage = m_document->viewport().pageNumber;
    if ( m_selected && m_selected->pageNumber() == newPage )
        return;

    // deselect previous thumbnail
    if ( m_selected )
        m_selected->setSelected( false );
    m_selected = 0;

    // select the page with viewport and ensure it's centered in the view
    m_vectorIndex = 0;
    QVector<ThumbnailWidget *>::const_iterator tIt = m_thumbnails.begin(), tEnd = m_thumbnails.end();
    for ( ; tIt != tEnd; ++tIt )
    {
        if ( (*tIt)->pageNumber() == newPage )
        {
            m_selected = *tIt;
            m_selected->setSelected( true );
            if ( Okular::Settings::syncThumbnailsViewport() )
            {
                int yOffset = qMax( viewport()->height() / 4, m_selected->height() / 2 );
                ensureVisible( 0, m_selected->pos().y() + m_selected->height()/2, 0, yOffset );
            }
            break;
        }
        m_vectorIndex++;
    }
}

void ThumbnailList::notifyPageChanged( int pageNumber, int changedFlags )
{
    static int interestingFlags = DocumentObserver::Pixmap | DocumentObserver::Bookmark | DocumentObserver::Highlights | DocumentObserver::Annotations;
    // only handle change notifications we are interested in
    if ( !( changedFlags & interestingFlags ) )
        return;

    // iterate over visible items: if page(pageNumber) is one of them, repaint it
    QList<ThumbnailWidget *>::const_iterator vIt = m_visibleThumbnails.begin(), vEnd = m_visibleThumbnails.end();
    for ( ; vIt != vEnd; ++vIt )
        if ( (*vIt)->pageNumber() == pageNumber )
        {
            (*vIt)->update();
            break;
        }
}

void ThumbnailList::notifyContentsCleared( int changedFlags )
{
    // if pixmaps were cleared, re-ask them
    if ( changedFlags & DocumentObserver::Pixmap )
        slotRequestVisiblePixmaps();
}

void ThumbnailList::notifyVisibleRectsChanged()
{
    bool found = false;
    const QVector<Okular::VisiblePageRect *> & visibleRects = m_document->visiblePageRects();
    QVector<ThumbnailWidget *>::const_iterator tIt = m_thumbnails.constBegin(), tEnd = m_thumbnails.constEnd();
    QVector<Okular::VisiblePageRect *>::const_iterator vEnd = visibleRects.end();
    for ( ; tIt != tEnd; ++tIt )
    {
        found = false;
        QVector<Okular::VisiblePageRect *>::const_iterator vIt = visibleRects.begin();
        for ( ; ( vIt != vEnd ) && !found; ++vIt )
        {
            if ( (*tIt)->pageNumber() == (*vIt)->pageNumber )
            {
                (*tIt)->setVisibleRect( (*vIt)->rect );
                found = true;
            }
        }
        if ( !found )
        {
            (*tIt)->setVisibleRect( Okular::NormalizedRect() );
        }
    }
}

bool ThumbnailList::canUnloadPixmap( int pageNumber ) const
{
    // if the thubnail 'pageNumber' is one of the visible ones, forbid unloading
    QList<ThumbnailWidget *>::const_iterator vIt = m_visibleThumbnails.begin(), vEnd = m_visibleThumbnails.end();
    for ( ; vIt != vEnd; ++vIt )
        if ( (*vIt)->pageNumber() == pageNumber )
            return false;
    // if hidden permit unloading
    return true;
}
//END DocumentObserver inherited methods 


void ThumbnailList::updateWidgets()
{
    // find all widgets that intersects the viewport and update them
    QRect viewportRect = viewport()->rect().translated( viewport()->pos() );
    QList<ThumbnailWidget *>::const_iterator vIt = m_visibleThumbnails.begin(), vEnd = m_visibleThumbnails.end();
    for ( ; vIt != vEnd; ++vIt )
    {
        ThumbnailWidget * t = *vIt;
        QRect thumbRect = t->rect().translated( m_pagesWidget->mapToParent( t->pos() ) );
        // update only the exposed area of the widget (saves pixels..)
        QRect relativeRect = thumbRect.intersect( viewport()->rect() );
        if ( !relativeRect.isValid() )
            continue;
        t->update( relativeRect );
    }
}

void ThumbnailList::forwardClick( const Okular::Page * p, const QPoint & t, Qt::MouseButton button )
{
    if ( button == Qt::RightButton )
        emit rightClick( p, t );
    else if ( button == Qt::LeftButton )
    {
        if ( m_document->viewport().pageNumber != p->number() )
            m_document->setViewportPage( p->number() );
    }
}

void ThumbnailList::forwardTrack( const Okular::Page * p, const QPoint &d, const QPoint &s )
{
    Okular::DocumentViewport vp=m_document->viewport();

    QVector< Okular::VisiblePageRect * > vVpr = m_document->visiblePageRects();

    QVector< Okular::VisiblePageRect * >::const_iterator vIt = vVpr.begin();
    QVector< Okular::VisiblePageRect * >::const_iterator vEnd = vVpr.end();
    for ( ; vIt != vEnd; ++vIt )
    {
        Okular::VisiblePageRect *vpr = ( *vIt );
        if( vpr->pageNumber == p->number() )
        {
            double w = vpr->rect.right - vpr->rect.left,
            h = vpr->rect.bottom - vpr->rect.top,
            deltaX = d.x()*w/s.x(),
            deltaY = d.y()*h/s.y();

            vp.rePos.normalizedX -= deltaX;
            vp.rePos.normalizedY -= deltaY;

            if( !vp.rePos.enabled )
            {
                vp.rePos.enabled = true;
                vp.rePos.normalizedY += h/2;
            }
            m_document->setViewport( vp );
        }
    }
}

void ThumbnailList::forwardZoom( const Okular::Page *, int i )
{
    m_document->setZoom( i );
}

const QPixmap * ThumbnailList::getBookmarkOverlay() const
{
    return m_bookmarkOverlay;
}

void ThumbnailList::slotFilterBookmarks( bool filterOn )
{
    // save state
    Okular::Settings::setFilterBookmarks( filterOn );
    Okular::Settings::self()->writeConfig();
    // ask for the 'notifySetup' with a little trick (on reinsertion the
    // document sends the list again)
    m_document->removeObserver( this );
    m_document->addObserver( this );
}


//BEGIN widget events 
void ThumbnailList::keyPressEvent( QKeyEvent * keyEvent )
{
    if ( m_thumbnails.count() < 1 )
        return keyEvent->ignore();

    int nextPage = -1;
    if ( keyEvent->key() == Qt::Key_Up )
    {
        if ( !m_selected )
            nextPage = 0;
        else if ( m_vectorIndex > 0 )
            nextPage = m_thumbnails[ m_vectorIndex - 1 ]->pageNumber();
    }
    else if ( keyEvent->key() == Qt::Key_Down )
    {
        if ( !m_selected )
            nextPage = 0;
        else if ( m_vectorIndex < (int)m_thumbnails.count() - 1 )
            nextPage = m_thumbnails[ m_vectorIndex + 1 ]->pageNumber();
    }
    else if ( keyEvent->key() == Qt::Key_PageUp )
        verticalScrollBar()->triggerAction( QScrollBar::SliderPageStepSub );
    else if ( keyEvent->key() == Qt::Key_PageDown )
        verticalScrollBar()->triggerAction( QScrollBar::SliderPageStepAdd );
    else if ( keyEvent->key() == Qt::Key_Home )
        nextPage = m_thumbnails[ 0 ]->pageNumber();
    else if ( keyEvent->key() == Qt::Key_End )
        nextPage = m_thumbnails[ m_thumbnails.count() - 1 ]->pageNumber();

    if ( nextPage == -1 )
        return keyEvent->ignore();

    keyEvent->accept();
    if ( m_selected )
        m_selected->setSelected( false );
    m_selected = 0;
    m_document->setViewportPage( nextPage );
}

bool ThumbnailList::viewportEvent( QEvent * e )
{
    switch ( e->type() )
    {
        case QEvent::Resize:
        {
            viewportResizeEvent( (QResizeEvent*)e );
            break;
        }
        default:
            ;
    }
    return QScrollArea::viewportEvent( e );
}

void ThumbnailList::viewportResizeEvent( QResizeEvent * e )
{
    if ( m_thumbnails.count() < 1 || width() < 1 )
        return;

    // if width changed resize all the Thumbnails, reposition them to the
    // right place and recalculate the contents area
    if ( e->size().width() != e->oldSize().width() )
    {
        // runs the timer avoiding a thumbnail regeneration by 'contentsMoving'
        delayedRequestVisiblePixmaps( 2000 );

        // resize and reposition items
        int newWidth = contentsRect().width() - verticalScrollBar()->width();
        int newHeight = 0;
        QVector<ThumbnailWidget *>::const_iterator tIt = m_thumbnails.begin(), tEnd = m_thumbnails.end();
        for ( ; tIt != tEnd; ++tIt )
        {
            ThumbnailWidget *t = *tIt;
            t->move(0, newHeight);
            t->resizeFitWidth( newWidth );
            newHeight += t->height() + KDialog::spacingHint();
        }

        // update scrollview's contents size (sets scrollbars limits)
        newHeight -= KDialog::spacingHint();
        m_pagesWidget->resize( newWidth, newHeight );

        // ensure selected item remains visible
        if ( m_selected )
            ensureVisible( 0, m_selected->mapToParent( QPoint( 0, 0 ) ).y() + m_selected->height()/2, 0, viewport()->height()/2 );
    }
    else if ( e->size().height() <= e->oldSize().height() )
        return;

    // invalidate the bookmark overlay
    if ( m_bookmarkOverlay )
    {
        delete m_bookmarkOverlay;
        m_bookmarkOverlay = 0;
    }

    // update Thumbnails since width has changed or height has increased
    delayedRequestVisiblePixmaps( 500 );
}

void ThumbnailList::dragEnterEvent( QDragEnterEvent * ev )
{
    ev->accept();
}

void ThumbnailList::dropEvent( QDropEvent * ev )
{
    if (  KUrl::List::canDecode(  ev->mimeData() ) )
        emit urlDropped( KUrl::List::fromMimeData( ev->mimeData() ).first() );
}
//END widget events

//BEGIN internal SLOTS 
void ThumbnailList::slotRequestVisiblePixmaps( int /*newContentsY*/ )
{
    // if an update is already scheduled or the widget is hidden, don't proceed
    if ( (m_delayTimer && m_delayTimer->isActive()) || isHidden() )
        return;

    // scroll from the top to the last visible thumbnail
    m_visibleThumbnails.clear();
    QLinkedList< Okular::PixmapRequest * > requestedPixmaps;
    QVector<ThumbnailWidget *>::const_iterator tIt = m_thumbnails.begin(), tEnd = m_thumbnails.end();
    for ( ; tIt != tEnd; ++tIt )
    {
        ThumbnailWidget * t = *tIt;
        QRect thumbRect = t->rect().translated( m_pagesWidget->mapToParent( t->pos() ) );
        if ( !thumbRect.intersects( viewport()->rect() ) )
          continue;
        // add ThumbnailWidget to visible list
        m_visibleThumbnails.push_back( t );
        // if pixmap not present add it to requests
        if ( !t->page()->hasPixmap( THUMBNAILS_ID, t->pixmapWidth(), t->pixmapHeight() ) )
        {
            Okular::PixmapRequest * p = new Okular::PixmapRequest(
                    THUMBNAILS_ID, t->pageNumber(), t->pixmapWidth(), t->pixmapHeight(), THUMBNAILS_PRIO, true );
            requestedPixmaps.push_back( p );
        }
    }

    // actually request pixmaps
    if ( !requestedPixmaps.isEmpty() )
        m_document->requestPixmaps( requestedPixmaps );
}

void ThumbnailList::slotDelayTimeout()
{
    // resize the bookmark overlay
    delete m_bookmarkOverlay;
    int expectedWidth = viewport()->width() / 4;
    if ( expectedWidth > 10 )
        m_bookmarkOverlay = new QPixmap( DesktopIcon( "bookmark", expectedWidth ) );
    else
        m_bookmarkOverlay = 0;

    // request pixmaps
    slotRequestVisiblePixmaps();
}
//END internal SLOTS

void ThumbnailList::delayedRequestVisiblePixmaps( int delayMs )
{
    if ( !m_delayTimer )
    {
        m_delayTimer = new QTimer( this );
        m_delayTimer->setSingleShot( true );
        connect( m_delayTimer, SIGNAL( timeout() ), this, SLOT( slotDelayTimeout() ) );
    }
    m_delayTimer->start( delayMs );
}


/** ThumbnailWidget implementation **/

ThumbnailWidget::ThumbnailWidget( QWidget * parent, const Okular::Document * document, const Okular::Page * kp, ThumbnailList * tl )
    : QWidget( parent ), m_tl( tl ), m_document( document ), m_page( kp ),
    m_selected( false ), m_pixmapWidth( 10 ), m_pixmapHeight( 10 )
{
    m_labelNumber = m_page->number() + 1;
    m_labelHeight = QFontMetrics( font() ).height();
    setMouseTracking(true);
    mouseGrabPos.setX(0);
    mouseGrabPos.setY(0);

}

void ThumbnailWidget::resizeFitWidth( int width )
{
    m_pixmapWidth = width - m_margin;
    m_pixmapHeight = qRound( m_page->ratio() * (double)m_pixmapWidth );
    setFixedSize( QSize( width, heightHint() ) );
}

void ThumbnailWidget::setSelected( bool selected )
{
    // update selected state
    if ( m_selected != selected )
    {
        m_selected = selected;
        update();
    }
}

void ThumbnailWidget::setVisibleRect( const Okular::NormalizedRect & rect )
{
    if ( rect == m_visibleRect )
       return;

    m_visibleRect = rect;
    update();
}

QSize ThumbnailWidget::sizeHint() const
{
    return QSize( width(), heightHint() );
}

void ThumbnailWidget::mousePressEvent( QMouseEvent * e )
{
    QRect r = m_visibleRect.geometry( m_pixmapWidth, m_pixmapHeight );

    if ( e->button() != Qt::RightButton && r.contains( e->pos() ) )
    {
        mouseGrabPos = e->pos();
    }
    else
    {
        mouseGrabPos.setX( 0 );
        mouseGrabPos.setY( 0 );
    }
}

void ThumbnailWidget::mouseReleaseEvent( QMouseEvent * e )
{
    QRect r = m_visibleRect.geometry( m_pixmapWidth, m_pixmapHeight );
    if ( r.contains( e->pos() ) )
    {
        setCursor( Qt::OpenHandCursor );
    }
    else
    {
        setCursor( Qt::ArrowCursor );
        if ( mouseGrabPos.isNull() )
        {
            // don't handle the mouse click, forward it to the thumbnail list
            m_tl->forwardClick( m_page, e->globalPos(), e->button() );
        }
    }
    mouseGrabPos.setX( 0 );
    mouseGrabPos.setY( 0 );
}

void ThumbnailWidget::mouseMoveEvent( QMouseEvent * e )
{
    QRect r = m_visibleRect.geometry( m_pixmapWidth, m_pixmapHeight );
    if ( r.contains( e->pos()-QPoint( m_margin / 2, m_margin / 2 ) ) )
    {
        if (!mouseGrabPos.isNull())
        {
            setCursor( Qt::ClosedHandCursor );
            QPoint mousePos = e->pos();
            QPoint delta = mouseGrabPos - mousePos;
            // don't handle the mouse move, forward it to the thumbnail list
            m_tl->forwardTrack( m_page, delta, QPoint( r.width(), r.height() ) );
            mouseGrabPos = e->pos();
        }
        else
        {
            setCursor( Qt::OpenHandCursor );
        }
    }
    else
    {
        setCursor( Qt::ArrowCursor );
    }
}

void ThumbnailWidget::wheelEvent( QWheelEvent * e )
{
    QRect r = m_visibleRect.geometry( m_pixmapWidth, m_pixmapHeight );

    if ( r.contains( e->pos() - QPoint( m_margin / 2, m_margin / 2 ) ) && e->orientation() == Qt::Vertical && e->modifiers() == Qt::ControlModifier )
    {
        m_tl->forwardZoom( m_page, e->delta() );
    }
    else
    {
        e->ignore();
    }
}

void ThumbnailWidget::contextMenuEvent( QContextMenuEvent * e )
{
    // don't handle the mouse click, forward it to the thumbnail list
    m_tl->forwardClick( m_page, e->globalPos(), Qt::RightButton );
}

void ThumbnailWidget::paintEvent( QPaintEvent * e )
{
    int width = m_pixmapWidth + m_margin;
    QRect clipRect = e->rect();
    if ( !clipRect.isValid() )
        return;
    QPainter p( this );
    QPalette pal = palette();

    // draw the bottom label + highlight mark
    QColor fillColor = m_selected ? pal.color( QPalette::Active, QPalette::Highlight ) : pal.color( QPalette::Active, QPalette::Base );
    p.fillRect( clipRect, fillColor );
    p.setPen( m_selected ? pal.color( QPalette::Active, QPalette::HighlightedText ) : pal.color( QPalette::Active, QPalette::Text ) );
    p.drawText( 0, m_pixmapHeight + m_margin, width, m_labelHeight, Qt::AlignCenter, QString::number( m_labelNumber ) );

    // draw page outline and pixmap
    if ( clipRect.top() < m_pixmapHeight + m_margin )
    {
        // if page is bookmarked draw a colored border
        bool isBookmarked = m_document->isBookmarked( pageNumber() );
        // draw the inner rect
        p.setPen( isBookmarked ? QColor( 0xFF8000 ) : Qt::black );
        p.drawRect( m_margin/2 - 1, m_margin/2 - 1, m_pixmapWidth + 1, m_pixmapHeight + 1 );
        // draw the clear rect
        p.setPen( isBookmarked ? QColor( 0x804000 ) : pal.color( QPalette::Active, QPalette::Base ) );
        // draw the bottom and right shadow edges
        if ( !isBookmarked )
        {
            int left, right, bottom, top;
            left = m_margin/2 + 1;
            right = m_margin/2 + m_pixmapWidth + 1;
            bottom = m_pixmapHeight + m_margin/2 + 1;
            top = m_margin/2 + 1;
            p.setPen( Qt::gray );
            p.drawLine( left, bottom, right, bottom );
            p.drawLine( right, top, right, bottom );
        }

        // draw the page using the shared PagePainter class
        p.translate( m_margin/2, m_margin/2 );
        clipRect.translate( -m_margin/2, -m_margin/2 );
        clipRect = clipRect.intersect( QRect( 0, 0, m_pixmapWidth, m_pixmapHeight ) );
        if ( clipRect.isValid() )
        {
            int flags = PagePainter::Accessibility | PagePainter::Highlights |
                        PagePainter::Annotations;
            PagePainter::paintPageOnPainter( &p, m_page, THUMBNAILS_ID, flags,
                                             m_pixmapWidth, m_pixmapHeight, clipRect );
        }

        if ( !m_visibleRect.isNull() )
        {
            p.save();
            p.setPen( QColor( 255, 255, 0, 200 ) );
            p.setBrush( QColor( 0, 0, 0, 100 ) );
            p.drawRect( m_visibleRect.geometry( m_pixmapWidth, m_pixmapHeight ) );
            p.restore();
        }

        // draw the bookmark overlay on the top-right corner
        const QPixmap * bookmarkPixmap = m_tl->getBookmarkOverlay();
        if ( isBookmarked && bookmarkPixmap )
        {
            int pixW = bookmarkPixmap->width(),
                pixH = bookmarkPixmap->height();
            clipRect = clipRect.intersect( QRect( m_pixmapWidth - pixW, 0, pixW, pixH ) );
            if ( clipRect.isValid() )
                p.drawPixmap( m_pixmapWidth - pixW, -pixH/8, *bookmarkPixmap );
        }
    }
}


/** ThumbnailsController implementation **/

#define FILTERB_ID  1

ThumbnailController::ThumbnailController( QWidget * parent, ThumbnailList * list )
    : QToolBar( parent )
{
    setObjectName( "ThumbsControlBar" );
    // change toolbar appearance
    setIconSize( QSize( 16, 16 ) );
    setMovable( false );
    QSizePolicy sp = sizePolicy();
    sp.setVerticalPolicy( QSizePolicy::Minimum );
    setSizePolicy( sp );

    // insert a togglebutton [show only bookmarked pages]
    //insertSeparator();
    QAction * showBoomarkOnlyAction = addAction(
        KIcon( "bookmark" ), i18n( "Show bookmarked pages only" ) );
    showBoomarkOnlyAction->setCheckable( true );
    connect( showBoomarkOnlyAction, SIGNAL( toggled( bool ) ), list, SLOT( slotFilterBookmarks( bool ) ) );
    showBoomarkOnlyAction->setChecked( Okular::Settings::filterBookmarks() );
    //insertLineSeparator();
}


#include "thumbnaillist.moc"
