/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <math.h>

#include "PDFDoc.h"

#include "thumbnailgenerator.h"
#include "thumbnaillist.h"
#include "thumbnail.h"

#include "page.h"

ThumbnailList::ThumbnailList(QWidget *parent, KPDFDocument *document)
	: QScrollView(parent), m_document(document), m_selected(0), m_oldWidth(0), m_oldHeight(0)
{
	// set scrollbars
	setHScrollBarMode( QScrollView::AlwaysOff );
	setVScrollBarMode( QScrollView::AlwaysOn );

	// dealing with large areas so enable clipper
	enableClipper( true );

	// can be focused by tab and mouse click and grabs key events
	viewport()->setFocusPolicy( StrongFocus );
	viewport()->setInputMethodEnabled( true );

	// set contents background to the 'base' color
	viewport()->setPaletteBackgroundColor( palette().active().base() );

	connect( this, SIGNAL(contentsMoving(int, int)), this, SLOT(slotRequestThumbnails(int, int)) );
}


void ThumbnailList::pageSetup( const QValueList<int> & pages )
{
	// delete all the Thumbnails
	QValueVector<Thumbnail *>::iterator thumbIt = thumbnails.begin();
	QValueVector<Thumbnail *>::iterator thumbEnd = thumbnails.end();
	for (; thumbIt != thumbEnd; ++thumbIt)
		delete (*thumbIt);
	thumbnails.clear();
	m_selected = 0;

	if ( pages.count() < 1 )
		return;

	// generate a new 'Thumbnail' objects for every page
	Thumbnail *t;
	int width = clipper()->width(),
	    totalHeight = 0;
	QValueList<int>::const_iterator pageIt = pages.begin();
	QValueList<int>::const_iterator pageEnd = pages.end();
	for (; pageIt != pageEnd ; ++pageIt)
	{
		t = new Thumbnail( viewport(), viewport()->paletteBackgroundColor(), m_document->page(*pageIt) );
		// add to the scrollview
		addChild( t, 0, totalHeight );
		// add to the internal queue
		thumbnails.push_back( t );
		// update total height (asking widget its own height)
		totalHeight += t->setThumbnailWidth( width );
		t->show();
	}

	// update scrollview's contents size (sets scrollbars limits)
	resizeContents( width, totalHeight );

	// request for thumbnail generation
	slotRequestThumbnails();
}

void ThumbnailList::pageSetCurrent( int pageNumber, float /*position*/ )
{
	// deselect previous page
	if ( m_selected )
		m_selected->setSelected( false );
	m_selected = 0;

	// select next page
	QValueVector<Thumbnail *>::iterator thumbIt = thumbnails.begin();
	QValueVector<Thumbnail *>::iterator thumbEnd = thumbnails.end();
	for (; thumbIt != thumbEnd; ++thumbIt)
		if ( (*thumbIt)->pageNumber() == pageNumber )
		{
			m_selected = *thumbIt;
			m_selected->setSelected( true );
			int itemTop = childY( m_selected );
			int itemHeight = m_selected->height();
			ensureVisible( 0, itemTop + itemHeight/2, 0, itemHeight/2 );
			break;
		}
}

void ThumbnailList::notifyThumbnailChanged( int pageNumber )
{
	QValueVector<Thumbnail *>::iterator thumbIt = thumbnails.begin();
	QValueVector<Thumbnail *>::iterator thumbEnd = thumbnails.end();
	for (; thumbIt != thumbEnd; ++thumbIt)
		if ( (*thumbIt)->pageNumber() == pageNumber )
		{
			(*thumbIt)->update();
			break;
		}
}


void ThumbnailList::slotRequestThumbnails( int /*newContentsX*/, int newContentsY )
{
	int vHeight = visibleHeight(),
	    vOffset = newContentsY == -1 ? contentsY() : newContentsY;

	// scroll from the top to the last visible thumbnail
	QValueVector<Thumbnail *>::iterator thumbIt = thumbnails.begin();
	QValueVector<Thumbnail *>::iterator thumbEnd = thumbnails.end();
	for (; thumbIt != thumbEnd; ++thumbIt)
	{
		Thumbnail * t = *thumbIt;
		int top = childY( t ) - vOffset;
		if ( top > vHeight )
			break;
		else if ( top + t->height() > 0 )
			m_document->makeThumbnail( t->pageNumber(), t->width(), t->height() );
	}
}

void ThumbnailList::viewportResizeEvent(QResizeEvent *e)
{
	// do not dispatch resize event (QTable::viewportResizeEvent(e)) so we
	// can refresh thumbnail size here

	// check if *width* changed (for rescaling thumbnails)
	int newWidth = e->size().width();
	if ( newWidth != m_oldWidth )
	{
		m_oldWidth = newWidth;

		viewport()->setUpdatesEnabled( false );
		setUpdatesEnabled( false );
		int totalHeight = 0;
		QValueVector<Thumbnail *>::iterator thumbIt = thumbnails.begin();
		QValueVector<Thumbnail *>::iterator thumbEnd = thumbnails.end();
		for (; thumbIt != thumbEnd; ++thumbIt)
		{
			Thumbnail *t = *thumbIt;
			moveChild( t, 0, totalHeight );
			totalHeight += t->setThumbnailWidth( newWidth );
		}
		setUpdatesEnabled( true );
		viewport()->setUpdatesEnabled( true );

		// update scrollview's contents size (sets scrollbars limits)
		resizeContents( newWidth, totalHeight );
/*
		int rows = numRows();
		for(int i = 0; i < rows; ++i)
			setRowHeight(i, static_cast<Thumbnail *>(cellWidget(i, 0))->setThumbnailWidth( newWidth ));
*/
	// that if are here to avoid recursive resizing of death
	// where the user makes the window smaller, that makes appear
	// the vertical scrollbar, that makes thumbnails smaller, and
	// while they get smaller the vertical scrollbar is not needed 
	// and ...
	// ... it also works for when the user makes the window larger
	// and then the scrollbar disappears but that makes thumbnails
	// larger and then scrollbar reappears and ...
/*
	Thumbnail *t;
	if (numRows() == 0) return;
	
	t = dynamic_cast<Thumbnail *>(cellWidget(0, 0));
	if (size().height() <= m_heightLimit)
	{
		if (t->getImageHeight() > (int)(visibleWidth()*m_ar))
		{
			setColumnWidth(0, visibleWidth());
			resizeThumbnails();
		}
	}
	else
	{
		if (visibleWidth() != columnWidth(0))
		{
			setColumnWidth(0, visibleWidth());
			resizeThumbnails();
			if (size().height() > m_heightLimit && verticalScrollBar() -> isVisible())
			{
				m_heightLimit = (int) ceil(numRows() * ((visibleWidth() + verticalScrollBar() -> width()) * m_ar + t -> labelSizeHintHeight()));
			}
		}
	}

*/
		// update thumbnails
		slotRequestThumbnails();
	}

	// check if height increased
	else if ( e->size().height() > m_oldHeight )
	{
		m_oldHeight = e->size().height();
		// only update thumbnails
		slotRequestThumbnails();
	}
}

void ThumbnailList::contentsMousePressEvent( QMouseEvent * e )
{
	int clickY = e->y();

	// find the Thumbnail over which the click is done
	QValueVector<Thumbnail *>::iterator thumbIt = thumbnails.begin();
	QValueVector<Thumbnail *>::iterator thumbEnd = thumbnails.end();
	for (; thumbIt != thumbEnd; ++thumbIt)
	{
		Thumbnail * t = *thumbIt;
		int childTop = childY(t);
		if ( clickY > childTop && clickY < (childTop + t->height()) )
		{
			m_document->slotSetCurrentPage( t->pageNumber() );
			break;
		}
	}
}

void ThumbnailList::keyPressEvent( QKeyEvent * keyEvent )
{
	int pageNumber = m_selected->pageNumber();
	if ( keyEvent->key() == Key_Up )
	{
		if ( !m_selected )
			m_document->slotSetCurrentPage( 0 );
		else if ( m_selected->pageNumber() > 0 )
			m_document->slotSetCurrentPage( pageNumber - 1 );
	}
	else if ( keyEvent->key() == Key_Down )
	{
		if ( !m_selected )
			m_document->slotSetCurrentPage( 0 );
		else if ( m_selected->pageNumber() < (int)m_document->pages() - 1 )
			m_document->slotSetCurrentPage( pageNumber + 1 );
	}
}


/** TO BE IMPORTED:
	void generateThumbnails(PDFDoc *doc);
	void stopThumbnailGeneration();
protected slots:
	void customEvent(QCustomEvent *e);
private slots:
	void changeSelected(int i);
	void emitClicked(int i);
signals:
	void clicked(int);
private:
	void generateNextThumbnail();
	ThumbnailGenerator *m_tg;

	void resizeThumbnails();
	int m_nextThumbnail;
	bool m_ignoreNext;

DELETE:
if (m_tg)
{
	m_tg->wait();
	delete m_tg;
}

void ThumbnailList::generateThumbnails(PDFDoc *doc)
{
	m_nextThumbnail = 1;
	m_doc = doc;
	generateNextThumbnail();
}

void ThumbnailList::generateNextThumbnail()
{
	if (m_tg)
	{
		m_tg->wait();
		delete m_tg;
	}
	m_tg = new ThumbnailGenerator(m_doc, m_docMutex, m_nextThumbnail, QPaintDevice::x11AppDpiX(), this);
	m_tg->start();
}


void ThumbnailList::stopThumbnailGeneration()
{
	if (m_tg)
	{
		m_ignoreNext = true;
		m_tg->wait();
		delete m_tg;
		m_tg = 0;
	}
}


void ThumbnailList::customEvent(QCustomEvent *e)
{
	if (e->type() == 65432 && !m_ignoreNext)
	{
		QImage *i =  (QImage*)(e -> data());
		
		setThumbnail(m_nextThumbnail, i);
		m_nextThumbnail++;
		if (m_nextThumbnail <= m_doc->getNumPages()) generateNextThumbnail();
		else
		{
			m_tg->wait();
			delete m_tg;
			m_tg = 0;
		}
	}
	m_ignoreNext = false;
}
*/

#include "thumbnaillist.moc"
