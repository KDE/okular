/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qtimer.h>
#include <qpainter.h>
#include <klocale.h>
#include <kurl.h>
#include <kurldrag.h>
#include <kaction.h>
#include <kactioncollection.h>

#include "thumbnaillist.h"
#include "page.h"

// ThumbnailWidget represents a single thumbnail in the ThumbnailList
class ThumbnailWidget : public QWidget
{
    public:
        ThumbnailWidget( QWidget * parent, const KPDFPage * page );

        // set internal parameters to fit the page in the given width
        void resizeFitWidth( int width );
        // set thumbnail's selected state
        void setSelected( bool selected );

        // query methods
        int heightHint() const { return m_pixmapHeight + m_labelHeight + 4; }
        int pixmapWidth() const { return m_pixmapWidth; }
        int pixmapHeight() const { return m_pixmapHeight; }
        int pageNumber() const { return m_page->number(); }

    protected:
        void paintEvent(QPaintEvent *);

    private:
        const KPDFPage * m_page;
        bool m_selected;
        int m_pixmapWidth, m_pixmapHeight;
        int m_labelHeight, m_labelNumber;
};


/** ThumbnailList implementation **/

ThumbnailList::ThumbnailList( QWidget *parent, KPDFDocument *document )
	: QScrollView( parent, "KPDF::Thumbnails", WNoAutoErase | WStaticContents ),
	m_document( document ), m_selected( 0 ), m_delayTimer( 0 )
{
	// set scrollbars
	setHScrollBarMode( QScrollView::AlwaysOff );
	setVScrollBarMode( QScrollView::AlwaysOn );

	// dealing with large areas so enable clipper
	enableClipper( true );

	// widget setup: can be focused by tab and mouse click (not wheel)
	viewport()->setFocusProxy( this );
	viewport()->setFocusPolicy( StrongFocus );
	viewport()->setPaletteBackgroundColor( Qt::gray );
	setResizePolicy( Manual );
	setAcceptDrops( true );
	setDragAutoScroll( false );

	// set contents background to the 'base' color
	viewport()->setPaletteBackgroundColor( palette().active().base() );

	setFrameStyle( StyledPanel | Raised );
	connect( this, SIGNAL(contentsMoving(int, int)), this, SLOT(slotRequestPixmaps(int, int)) );
}


//BEGIN KPDFDocumentObserver inherited methods 
void ThumbnailList::pageSetup( const QValueVector<KPDFPage*> & pages, bool /*documentChanged*/ )
{
	// delete all the Thumbnails
	QValueVector<ThumbnailWidget *>::iterator thumbIt = m_thumbnails.begin();
	QValueVector<ThumbnailWidget *>::iterator thumbEnd = m_thumbnails.end();
	for ( ; thumbIt != thumbEnd; ++thumbIt )
		delete *thumbIt;
	m_thumbnails.clear();
	m_selected = 0;

	if ( pages.count() < 1 )
	{
		resizeContents( 0, 0 );
		return;
	}

	//FIXME change this quick fix (lines that follows). Check if filtering:
	bool skipCheck = true;
	for ( uint i = 0; i < pages.count(); i++ )
        if ( pages[i]->attributes() & KPDFPage::Highlight )
			skipCheck = false;

	// generate Thumbnails for the given set of pages
	ThumbnailWidget *t;
	int width = clipper()->width(),
	    totalHeight = 0;
	QValueVector<KPDFPage*>::const_iterator pageIt = pages.begin();
	QValueVector<KPDFPage*>::const_iterator pageEnd = pages.end();
	for (; pageIt != pageEnd ; ++pageIt)
        if ( skipCheck || ( (*pageIt)->attributes() & KPDFPage::Highlight ) ) {
			t = new ThumbnailWidget( viewport(), *pageIt );
			t->setFocusProxy( this );
			// add to the scrollview
			addChild( t, 0, totalHeight );
			// add to the internal queue
			m_thumbnails.push_back( t );
			// update total height (asking widget its own height)
			t->resizeFitWidth( width );
			totalHeight += t->heightHint() + 4;
			t->show();
		}

	// update scrollview's contents size (sets scrollbars limits)
	resizeContents( width, totalHeight );

	// request for thumbnail generation
	requestPixmaps( 200 );
}

void ThumbnailList::pageSetCurrent( int pageNumber, const QRect & /*viewport*/ )
{
	// deselect previous thumbnail
	if ( m_selected )
		m_selected->setSelected( false );
	m_selected = 0;

	// select next page
	m_vectorIndex = 0;
	QValueVector<ThumbnailWidget *>::iterator thumbIt = m_thumbnails.begin();
	QValueVector<ThumbnailWidget *>::iterator thumbEnd = m_thumbnails.end();
	for (; thumbIt != thumbEnd; ++thumbIt)
	{
		if ( (*thumbIt)->pageNumber() == pageNumber )
		{
			m_selected = *thumbIt;
			m_selected->setSelected( true );
			ensureVisible( 0, childY( m_selected ) + m_selected->height()/2, 0, visibleHeight()/2 );
			//non-centered version: ensureVisible( 0, itemTop + itemHeight/2, 0, itemHeight/2 );
			break;
		}
		m_vectorIndex++;
	}
}

void ThumbnailList::notifyPixmapChanged( int pageNumber )
{
	QValueVector<ThumbnailWidget *>::iterator thumbIt = m_thumbnails.begin(), thumbEnd = m_thumbnails.end();
	for (; thumbIt != thumbEnd; ++thumbIt)
		if ( (*thumbIt)->pageNumber() == pageNumber )
		{
			(*thumbIt)->update();
			break;
		}
}

void ThumbnailList::updateWidgets()
{
    // find all widgets that intersects the viewport and update them
    QRect viewportRect( contentsX(), contentsY(), visibleWidth(), visibleHeight() );
    QValueVector<ThumbnailWidget *>::iterator tIt = m_thumbnails.begin(), tEnd = m_thumbnails.end();
    for ( ; tIt != tEnd; ++tIt )
    {
        ThumbnailWidget * t = *tIt;
        QRect widgetRect( childX( t ), childY( t ), t->width(), t->height() );
        if ( viewportRect.intersects( widgetRect ) )
        {
            // update only the exposed area of the widget (saves pixels..)
            QRect relativeRect = viewportRect.intersect( widgetRect );
            relativeRect.moveBy( -widgetRect.left(), -widgetRect.top() );
            t->update( relativeRect );
        }
    }
}

void ThumbnailList::dragEnterEvent( QDragEnterEvent * ev )
{
	ev->accept();
}

void ThumbnailList::dropEvent( QDropEvent * ev )
{
	KURL::List lst;
	if (  KURLDrag::decode(  ev, lst ) )
		emit urlDropped( lst.first() );
}
//END KPDFDocumentObserver inherited methods 

//BEGIN widget events 
void ThumbnailList::keyPressEvent( QKeyEvent * keyEvent )
{
	if ( m_thumbnails.count() < 1 )
		return keyEvent->ignore();

	int nextPage = -1;
	if ( keyEvent->key() == Key_Up )
	{
		if ( !m_selected )
			nextPage = 0;
		else if ( m_vectorIndex > 0 )
			nextPage = m_thumbnails[ m_vectorIndex - 1 ]->pageNumber();
	}
	else if ( keyEvent->key() == Key_Down )
	{
		if ( !m_selected )
			nextPage = 0;
		else if ( m_vectorIndex < (int)m_thumbnails.count() - 1 )
			nextPage = m_thumbnails[ m_vectorIndex + 1 ]->pageNumber();
	}
	else if ( keyEvent->key() == Key_Home )
		nextPage = m_thumbnails[ 0 ]->pageNumber();
	else if ( keyEvent->key() == Key_End )
		nextPage = m_thumbnails[ m_thumbnails.count() - 1 ]->pageNumber();

	if ( nextPage == -1 )
		return keyEvent->ignore();

	keyEvent->accept();
	if ( m_selected )
		m_selected->setSelected( false );
	m_selected = 0;
	m_document->slotSetCurrentPage( nextPage );
}

void ThumbnailList::contentsMousePressEvent( QMouseEvent * e )
{
	if ( e->button() != Qt::LeftButton )
		return;
	int clickY = e->y();
	QValueVector<ThumbnailWidget *>::iterator thumbIt = m_thumbnails.begin();
	QValueVector<ThumbnailWidget *>::iterator thumbEnd = m_thumbnails.end();
	for ( ; thumbIt != thumbEnd; ++thumbIt )
	{
		ThumbnailWidget * t = *thumbIt;
		int childTop = childY(t);
		if ( clickY > childTop && clickY < (childTop + t->height()) )
		{
			m_document->slotSetCurrentPage( t->pageNumber() );
			break;
		}
	}
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
		requestPixmaps( 2000 );

		// resize and reposition items
		int totalHeight = 0,
		    newWidth = e->size().width();
		QValueVector<ThumbnailWidget *>::iterator thumbIt = m_thumbnails.begin();
		QValueVector<ThumbnailWidget *>::iterator thumbEnd = m_thumbnails.end();
		for ( ; thumbIt != thumbEnd; ++thumbIt )
		{
			ThumbnailWidget *t = *thumbIt;
			moveChild( t, 0, totalHeight );
			t->resizeFitWidth( newWidth );
			totalHeight += t->heightHint() + 4;
		}

		// update scrollview's contents size (sets scrollbars limits)
		resizeContents( newWidth, totalHeight );

		// ensure selected item remains visible
		if ( m_selected )
			ensureVisible( 0, childY( m_selected ) + m_selected->height()/2, 0, visibleHeight()/2 );
	}
	else if ( e->size().height() <= e->oldSize().height() )
		return;
	// update Thumbnails since width has changed or height has increased
	requestPixmaps( 500 );
}
//END widget events

//BEGIN internal SLOTS 
void ThumbnailList::slotRequestPixmaps( int /*newContentsX*/, int newContentsY )
{
	// if an update is already scheduled or the widget is hidden, don't proceed
	if ( (m_delayTimer && m_delayTimer->isActive()) || !isShown() )
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
}
//END internal SLOTS

void ThumbnailList::requestPixmaps( int delayMs )
{
	if ( !m_delayTimer )
	{
		m_delayTimer = new QTimer( this );
		connect( m_delayTimer, SIGNAL( timeout() ), this, SLOT( slotRequestPixmaps() ) );
	}
	m_delayTimer->start( delayMs, true );
}


/** ThumbnailWidget implementation **/

ThumbnailWidget::ThumbnailWidget( QWidget * parent, const KPDFPage * kp  )
    : QWidget( parent, 0, WNoAutoErase ), m_page( kp ),
    m_selected( false ), m_pixmapWidth( 10 ), m_pixmapHeight( 10 )
{
    m_labelNumber = m_page->number() + 1;
    m_labelHeight = QFontMetrics( font() ).height();
}

void ThumbnailWidget::resizeFitWidth( int width )
{
    m_pixmapWidth = width - 4;
    m_pixmapHeight = (int)(m_page->ratio() * m_pixmapWidth);
    resize( width, heightHint() );
}

void ThumbnailWidget::setSelected( bool selected )
{
    // update selected state
    if ( m_selected != selected )
    {
        m_selected = selected;
        update( 0, m_pixmapHeight + 4, width(), m_labelHeight );
    }
}

void ThumbnailWidget::paintEvent( QPaintEvent * e )
{
    int width = m_pixmapWidth + 4;
    QRect clipRect = e->rect();
    if ( !clipRect.isValid() )
        return;
    QPainter p( this );

    // draw the bottom label
    if ( clipRect.bottom() > m_pixmapHeight + 3 )
    {
        QColor fillColor = m_selected ? palette().active().highlight() : palette().active().base();
        p.fillRect( 0, m_pixmapHeight + 4, width, m_labelHeight, fillColor );
        p.drawText( 0, m_pixmapHeight + 4, width, m_labelHeight, Qt::AlignCenter, QString::number( m_labelNumber ) );
    }

    // draw page outline and pixmap
    if ( clipRect.top() < m_pixmapHeight + 4 )
    {
        p.drawRect( 1, 1, m_pixmapWidth + 2, m_pixmapHeight + 2 );
        p.setPen( palette().active().base() );
        p.drawRect( 0, 0, m_pixmapWidth + 4, m_pixmapHeight + 4 );
        p.setPen( Qt::gray );
        p.drawLine( 5, m_pixmapHeight + 3, m_pixmapWidth + 3, m_pixmapHeight + 3 );
        p.drawLine( m_pixmapWidth + 3, 5, m_pixmapWidth + 3, m_pixmapHeight + 3 );

        p.translate( 2, 2 );
        clipRect.moveBy( -2, -2 );
        clipRect = clipRect.intersect( QRect( 0, 0, m_pixmapWidth, m_pixmapHeight ) );
        if ( clipRect.isValid() )
        {
            int flags = PagePainter::Accessibility | PagePainter::Highlight;
            PagePainter::paintPageOnPainter( m_page, THUMBNAILS_ID, flags, &p,
                                             clipRect, m_pixmapWidth, m_pixmapHeight );
        }
    }
}

#include "thumbnaillist.moc"
