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

ThumbnailList::ThumbnailList(QWidget *parent, KPDFDocument *document) : QTable(parent), m_document(document)
{
	setNumCols(1);
	setColumnStretchable(0,true);
	setLeftMargin(0);
	setTopMargin(0);
	setHScrollBarMode(QScrollView::AlwaysOff);
	setVScrollBarMode(QScrollView::AlwaysOn); //TODO soon restore autoscaling behavior
	setReadOnly(true);
	m_selected = 0;

	connect(this, SIGNAL(currentChanged(int, int)), document, SLOT(slotSetCurrentPage(int)));
}

void ThumbnailList::pageSetup( const QValueList<int> & pages )
{
	// delete old 'Thumbnail' objects
	for (int i=0; i < numRows(); i++)
		clearCellWidget( i, 0 );

	// generate new 'Thumbnail' objects
	Thumbnail *t;
	//viewport()->setUpdatesEnabled(false);
	setNumRows( pages.count() );
	//viewport()->setUpdatesEnabled(true);
	uint i = 0;
	int width = columnWidth(0);
	QValueList<int>::const_iterator it = pages.begin();
	QValueList<int>::const_iterator end = pages.end();
	for (; it != end ; ++it)
	{
		int pageNumber = *it;
		t = new Thumbnail( this, QString::number(pageNumber+1), viewport()->paletteBackgroundColor(),
			(int)(width * m_document->page(pageNumber)->ratio()), width);
		setCellWidget(i, 0, t);
		setRowHeight(i++, t->sizeHint().height());
	}

	// request for thumbnail generation
	//FIXME document->requestThumbnail...
	//or generateThumbnails(d->pdfdoc);
}

void ThumbnailList::pageSetCurrent( int pageNumber, float /*position*/ )
{
	setCurrentCell( pageNumber, 0 );
printf("current:%d\n",pageNumber);
	Thumbnail *t;
	t = dynamic_cast<Thumbnail *>(cellWidget(m_selected, 0));
	if (t) t -> setSelected(false);
	m_selected = pageNumber;
	t = dynamic_cast<Thumbnail *>(cellWidget(m_selected, 0));
	if (t) t -> setSelected(true);
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

void ThumbnailList::notifyThumbnailChanged( int pageNumber )
{
	Thumbnail *t = dynamic_cast<Thumbnail *>(cellWidget(pageNumber, 0));
	if ( t && viewport()->rect().intersects( t->rect() ) )
		t->update();
}

void ThumbnailList::viewportResizeEvent(QResizeEvent *e)
{
	printf("%d\n",e->size().width());
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
	//THE "resizeThumbnails" method: ##IMPORT##
	Thumbnail *t;
	for(int i = 0; i < numRows(); ++i)
	{
		t = dynamic_cast<Thumbnail *>(cellWidget(i, 0));
		t->setImageSize((int)(visibleWidth()*m_ar), visibleWidth());
		setRowHeight(i, (int)(visibleWidth()*m_ar) + t->labelSizeHintHeight());
	}
*/
}

#include "thumbnaillist.moc"
