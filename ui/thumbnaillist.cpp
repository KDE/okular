/***************************************************************************
 *   Copyright (C) 2004-2006 by Albert Astals Cid <tsdgeos@terra.es>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qevent.h>
#include <qlayout.h>
#include <qtimer.h>
#include <qpainter.h>
#include <qscrollbar.h>
#include <klocale.h>
#include <kurl.h>
#include <kaction.h>
#include <kiconloader.h>
#include <kactioncollection.h>

// system includes
#include <math.h>

// local includes
#include "thumbnaillist.h"
#include "pagepainter.h"
#include "searchwidget.h"   // for SW_SEARCH_ID
#include "core/document.h"
#include "core/generator.h"
#include "core/page.h"
#include "settings.h"

// ThumbnailWidget represents a single thumbnail in the ThumbnailList
class ThumbnailWidget : public QWidget
{
    public:
        ThumbnailWidget( QWidget * parent, const KPDFPage * page, ThumbnailList * tl );

        // set internal parameters to fit the page in the given width
        void resizeFitWidth( int width );
        // set thumbnail's selected state
        void setSelected( bool selected );

        // query methods
        int heightHint() const { return m_pixmapHeight + m_labelHeight + m_margin; }
        int pixmapWidth() const { return m_pixmapWidth; }
        int pixmapHeight() const { return m_pixmapHeight; }
        int pageNumber() const { return m_page->number(); }
        const KPDFPage * page() const { return m_page; }
        QSize sizeHint() const;

    protected:
        void mouseReleaseEvent( QMouseEvent * e );
        void contextMenuEvent( QContextMenuEvent * e );
        void paintEvent(QPaintEvent *);

    private:
        // the margin around the widget
        static int const m_margin = 16;

        // used to access 'forwardClick( .. )' and 'getBookmarkOverlay()'
        ThumbnailList * m_tl;
        const KPDFPage * m_page;
        bool m_selected;
        int m_pixmapWidth, m_pixmapHeight;
        int m_labelHeight, m_labelNumber;
};


/** ThumbnailList implementation **/

ThumbnailList::ThumbnailList( QWidget *parent, KPDFDocument *document )
	: QScrollArea( parent ),
	m_document( document ), m_selected( 0 ), m_delayTimer( 0 ), m_bookmarkOverlay( 0 )
{
	setObjectName( "KPDF::Thumbnails" );
	// set scrollbars
	setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );

	setAttribute( Qt::WA_StaticContents );

	setAcceptDrops( true );

	// set contents background to the 'base' color
	viewport()->setPaletteBackgroundColor( palette().active().base() );

	m_pagesWidget = new QWidget();
	setWidget( m_pagesWidget );
	// widget setup: can be focused by tab and mouse click (not wheel)
	m_pagesWidget->setFocusPolicy( Qt::StrongFocus );
	m_pagesWidget->show();
	m_pagesWidget->setPaletteBackgroundColor( palette().active().base() );
	m_pagesLayout = new QVBoxLayout( m_pagesWidget );
	m_pagesLayout->setMargin( 0 );
	m_pagesLayout->setSpacing( 4 );

	setFrameStyle( StyledPanel | Raised );

	connect( verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(slotRequestVisiblePixmaps(int)) );
}

ThumbnailList::~ThumbnailList()
{
    m_document->removeObserver( this );
    delete m_bookmarkOverlay;
}

//BEGIN DocumentObserver inherited methods
void ThumbnailList::notifySetup( const QVector< KPDFPage * > & pages, bool /*documentChanged*/ )
{
	// delete all the Thumbnails
	QVector<ThumbnailWidget *>::iterator tIt = m_thumbnails.begin(), tEnd = m_thumbnails.end();
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
    //RESTORE THIS int flags = KpdfSettings::filterBookmarks() ? KPDFPage::Bookmark : KPDFPage::Highlight;

    // if no page matches filter rule, then display all pages
    QVector< KPDFPage * >::const_iterator pIt = pages.begin(), pEnd = pages.end();
    bool skipCheck = true;
    for ( ; pIt != pEnd ; ++pIt )
        //if ( (*pIt)->attributes() & flags )
        if ( (*pIt)->hasHighlights( SW_SEARCH_ID ) )
            skipCheck = false;

    // generate Thumbnails for the given set of pages
    int width = viewport()->width();
    for ( pIt = pages.begin(); pIt != pEnd ; ++pIt )
        //if ( skipCheck || (*pIt)->attributes() & flags )
        if ( skipCheck || (*pIt)->hasHighlights( SW_SEARCH_ID ) )
        {
            ThumbnailWidget * t = new ThumbnailWidget( widget(), *pIt, this );
            t->setFocusProxy( this );
            // add to the scrollview
            m_pagesLayout->addWidget( t );
            // add to the internal queue
            m_thumbnails.push_back( t );
            // update total height (asking widget its own height)
            t->resizeFitWidth( width );
            t->show();
        }

    // update scrollview's contents size (sets scrollbars limits)
    m_pagesWidget->resize( m_pagesWidget->sizeHint() );
    m_pagesLayout->update();

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
	QVector<ThumbnailWidget *>::iterator tIt = m_thumbnails.begin(), tEnd = m_thumbnails.end();
	for ( ; tIt != tEnd; ++tIt )
	{
		if ( (*tIt)->pageNumber() == newPage )
		{
			m_selected = *tIt;
			m_selected->setSelected( true );
			if ( KpdfSettings::syncThumbnailsViewport() )
			{
				int yOffset = qMax( viewport()->height() / 4, m_selected->height() / 2 );
				ensureVisible( 0, m_selected->pos().y() + m_selected->height()/2, 0, yOffset );
			}
			break;
		}
		m_vectorIndex++;
	}
}

void ThumbnailList::notifyPageChanged( int pageNumber, int /*changedFlags*/ )
{
    // only handle pixmap changed notifies (the only defined for now)
    //if ( !(changedFlags & DocumentObserver::Pixmap) )
    //    return;

    // iterate over visible items: if page(pageNumber) is one of them, repaint it
    QList<ThumbnailWidget *>::iterator vIt = m_visibleThumbnails.begin(), vEnd = m_visibleThumbnails.end();
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

bool ThumbnailList::canUnloadPixmap( int pageNumber )
{
    // if the thubnail 'pageNumber' is one of the visible ones, forbid unloading
    QList<ThumbnailWidget *>::iterator vIt = m_visibleThumbnails.begin(), vEnd = m_visibleThumbnails.end();
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
    QList<ThumbnailWidget *>::iterator vIt = m_visibleThumbnails.begin(), vEnd = m_visibleThumbnails.end();
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

void ThumbnailList::forwardClick( const KPDFPage * p, const QPoint & t, Qt::MouseButton button )
{
    if ( button == Qt::RightButton )
        emit rightClick( p, t );
    else if ( button == Qt::LeftButton )
    {
        if ( m_document->viewport().pageNumber != p->number() )
            m_document->setViewportPage( p->number() );
    }
}

const QPixmap * ThumbnailList::getBookmarkOverlay() const
{
    return m_bookmarkOverlay;
}

void ThumbnailList::slotFilterBookmarks( bool filterOn )
{
    // save state
    KpdfSettings::setFilterBookmarks( filterOn );
    KpdfSettings::writeConfig();
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
		QVector<ThumbnailWidget *>::iterator tIt = m_thumbnails.begin(), tEnd = m_thumbnails.end();
		for ( ; tIt != tEnd; ++tIt )
		{
			ThumbnailWidget *t = *tIt;
			t->resizeFitWidth( newWidth );
		}

		// update scrollview's contents size (sets scrollbars limits)
		m_pagesLayout->invalidate();
		m_pagesWidget->resize( m_pagesWidget->sizeHint() );

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
    QLinkedList< PixmapRequest * > requestedPixmaps;
    QVector<ThumbnailWidget *>::iterator tIt = m_thumbnails.begin(), tEnd = m_thumbnails.end();
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
            PixmapRequest * p = new PixmapRequest(
                    THUMBNAILS_ID, t->pageNumber(), t->pixmapWidth(), t->pixmapHeight(), 
                    t->page()->rotation(), THUMBNAILS_PRIO, true );
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
        m_bookmarkOverlay = new QPixmap( DesktopIcon( "attach", expectedWidth ) );
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
		connect( m_delayTimer, SIGNAL( timeout() ), this, SLOT( slotDelayTimeout() ) );
	}
	m_delayTimer->start( delayMs, true );
}


/** ThumbnailWidget implementation **/

ThumbnailWidget::ThumbnailWidget( QWidget * parent, const KPDFPage * kp, ThumbnailList * tl )
    : QWidget( parent ), m_tl( tl ), m_page( kp ),
    m_selected( false ), m_pixmapWidth( 10 ), m_pixmapHeight( 10 )
{
    m_labelNumber = m_page->number() + 1;
    m_labelHeight = QFontMetrics( font() ).height();
}

void ThumbnailWidget::resizeFitWidth( int width )
{
    m_pixmapWidth = width - m_margin;
    m_pixmapHeight = (int)round( m_page->ratio() * (double)m_pixmapWidth );
    resize( width, heightHint() );
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

QSize ThumbnailWidget::sizeHint() const
{
    return QSize( width(), heightHint() );
}

void ThumbnailWidget::mouseReleaseEvent( QMouseEvent * e )
{
    // don't handle the mouse click, forward it to the thumbnail list
    m_tl->forwardClick( m_page, e->globalPos(), e->button() );
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

    // draw the bottom label + highlight mark
    QColor fillColor = m_selected ? palette().active().highlight() : palette().active().base();
    p.fillRect( clipRect, fillColor );
    p.setPen( m_selected ? palette().active().highlightedText() : palette().active().text() );
    p.drawText( 0, m_pixmapHeight + m_margin, width, m_labelHeight, Qt::AlignCenter, QString::number( m_labelNumber ) );

    // draw page outline and pixmap
    if ( clipRect.top() < m_pixmapHeight + m_margin )
    {
        // if page is bookmarked draw a colored border
        bool isBookmarked = m_page->hasBookmark();
        // draw the inner rect
        p.setPen( isBookmarked ? QColor( 0xFF8000 ) : Qt::black );
        p.drawRect( m_margin/2 - 1, m_margin/2 - 1, m_pixmapWidth + 2, m_pixmapHeight + 2 );
        // draw the clear rect
        p.setPen( isBookmarked ? QColor( 0x804000 ) : palette().active().base() );
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

    // insert a togglebutton [show only bookmarked pages]
    //insertSeparator();
    QAction * showBoomarkOnlyAction = addAction(
        KIcon( "bookmark" ), i18n( "Show bookmarked pages only" ) );
    showBoomarkOnlyAction->setCheckable( true );
    connect( showBoomarkOnlyAction, SIGNAL( toggled( bool ) ), list, SLOT( slotFilterBookmarks( bool ) ) );
    showBoomarkOnlyAction->setChecked( KpdfSettings::filterBookmarks() );
    //insertLineSeparator();
}


#include "thumbnaillist.moc"
