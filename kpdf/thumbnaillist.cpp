/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qtimer.h>
#include <klocale.h>
#include <kconfigbase.h>
#include <kaction.h>
#include <kactioncollection.h>

#include "thumbnaillist.h"
#include "thumbnail.h"
#include "page.h"

ThumbnailList::ThumbnailList(QWidget *parent, KPDFDocument *document)
	: QScrollView(parent), m_document(document), m_selected(0), m_delayTimer(0)
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

	setFrameStyle( StyledPanel | Raised );
	connect( this, SIGNAL(contentsMoving(int, int)), this, SLOT(slotRequestThumbnails(int, int)) );
}

void ThumbnailList::setupActions( KActionCollection * ac, KConfigGroup * config )
{
	KToggleAction * show = new KToggleAction( i18n( "Show &Page List" ), 0, ac, "show_page_list" );
	show->setCheckedState(i18n("Hide &Page List"));
	connect( show, SIGNAL( toggled( bool ) ), SLOT( setShown( bool ) ) );

	show->setChecked( config->readBoolEntry( "ShowPageList", true ) );
	setShown( show->isChecked() );
}

void ThumbnailList::saveSettings( KConfigGroup * config )
{
	config->writeEntry( "ShowPageList", isShown() );
}

//BEGIN KPDFDocumentObserver inherited methods 
void ThumbnailList::pageSetup( const QValueList<int> & pages )
{
	// delete all the Thumbnails
	QValueVector<Thumbnail *>::iterator thumbIt = m_thumbnails.begin();
	QValueVector<Thumbnail *>::iterator thumbEnd = m_thumbnails.end();
	for ( ; thumbIt != thumbEnd; ++thumbIt )
		delete *thumbIt;
	m_thumbnails.clear();
	m_selected = 0;

	if ( pages.count() < 1 )
	{
		resizeContents( 0, 0 );
		return;
	}

	// generate Thumbnails for the given set of pages
	Thumbnail *t;
	int width = clipper()->width(),
	    totalHeight = 0;
	QValueList<int>::const_iterator pageIt = pages.begin();
	QValueList<int>::const_iterator pageEnd = pages.end();
	for (; pageIt != pageEnd ; ++pageIt)
	{
		t = new Thumbnail( viewport(), m_document->page(*pageIt) );
		// add to the scrollview
		addChild( t, 0, totalHeight );
		// add to the internal queue
		m_thumbnails.push_back( t );
		// update total height (asking widget its own height)
		totalHeight += t->setThumbnailWidth( width );
		t->show();
	}

	// update scrollview's contents size (sets scrollbars limits)
	resizeContents( width, totalHeight );

	// request for thumbnail generation
	requestThumbnails( 200 );
}

void ThumbnailList::pageSetCurrent( int pageNumber, float /*position*/ )
{
	// deselect previous thumbnail
	if ( m_selected )
		m_selected->setSelected( false );
	m_selected = 0;

	// select next page
	m_vectorIndex = 0;
	QValueVector<Thumbnail *>::iterator thumbIt = m_thumbnails.begin();
	QValueVector<Thumbnail *>::iterator thumbEnd = m_thumbnails.end();
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

void ThumbnailList::notifyThumbnailChanged( int pageNumber )
{
	QValueVector<Thumbnail *>::iterator thumbIt = m_thumbnails.begin();
	QValueVector<Thumbnail *>::iterator thumbEnd = m_thumbnails.end();
	for (; thumbIt != thumbEnd; ++thumbIt)
		if ( (*thumbIt)->pageNumber() == pageNumber )
		{
			(*thumbIt)->update();
			break;
		}
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
	int clickY = e->y();
	QValueVector<Thumbnail *>::iterator thumbIt = m_thumbnails.begin();
	QValueVector<Thumbnail *>::iterator thumbEnd = m_thumbnails.end();
	for ( ; thumbIt != thumbEnd; ++thumbIt )
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

void ThumbnailList::viewportResizeEvent(QResizeEvent *e)
{
	if ( m_thumbnails.count() < 1 )
		return;
	// if width changed resize all the Thumbnails, reposition them to the
	// right place and recalculate the contents area
	if ( e->size().width() != e->oldSize().width() )
	{
		// runs the timer avoiding a thumbnail regeneration by 'contentsMoving'
		requestThumbnails( 2000 );

		// resize and reposition items
		int totalHeight = 0,
		    newWidth = e->size().width();
		QValueVector<Thumbnail *>::iterator thumbIt = m_thumbnails.begin();
		QValueVector<Thumbnail *>::iterator thumbEnd = m_thumbnails.end();
		for ( ; thumbIt != thumbEnd; ++thumbIt )
		{
			Thumbnail *t = *thumbIt;
			moveChild( t, 0, totalHeight );
			totalHeight += t->setThumbnailWidth( newWidth );
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
	requestThumbnails( 500 );
}
//END widget events 

//BEGIN internal SLOTS 
void ThumbnailList::slotRequestThumbnails( int /*newContentsX*/, int newContentsY )
{
	// an update is already scheduled, so don't proceed
	if ( m_delayTimer && m_delayTimer->isActive() )
		return;

	int vHeight = visibleHeight(),
	    vOffset = newContentsY == -1 ? contentsY() : newContentsY;

	// scroll from the top to the last visible thumbnail
	QValueVector<Thumbnail *>::iterator thumbIt = m_thumbnails.begin();
	QValueVector<Thumbnail *>::iterator thumbEnd = m_thumbnails.end();
	for ( ; thumbIt != thumbEnd; ++thumbIt )
	{
		Thumbnail * t = *thumbIt;
		int top = childY( t ) - vOffset;
		if ( top > vHeight )
			break;
		else if ( top + t->height() > 0 )
			m_document->requestThumbnail( t->pageNumber(), t->previewWidth(), t->previewHeight(), true );
	}
}
//END internal SLOTS

void ThumbnailList::requestThumbnails( int delayMs )
{
	if ( !m_delayTimer )
	{
		m_delayTimer = new QTimer( this );
		connect( m_delayTimer, SIGNAL( timeout() ), this, SLOT( slotRequestThumbnails() ) );
	}
	m_delayTimer->start( delayMs, true );
}

#include "thumbnaillist.moc"
