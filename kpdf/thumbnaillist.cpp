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

ThumbnailList::ThumbnailList(QWidget *parent, QMutex *docMutex) : QTable(parent), m_tg(0), m_doc(0), m_docMutex(docMutex)
{
    setNumCols(1);
    setLeftMargin(0);
    setTopMargin(0);
    setHScrollBarMode(QScrollView::AlwaysOff);
    m_selected = 0;
    
    connect(this, SIGNAL(pressed(int, int, int, const QPoint&)), this, SIGNAL(clicked(int)));
    connect(this, SIGNAL(currentChanged(int, int)), this, SIGNAL(clicked(int)));
    connect(this, SIGNAL(currentChanged(int, int)), this, SLOT(changeSelected(int)));
}

ThumbnailList::~ThumbnailList()
{
    if (m_tg)
    {
        m_tg->wait();
        delete m_tg;
    }
}

void ThumbnailList::setCurrentItem(int i)
{
    setCurrentCell(i, 0); 
    changeSelected(i);
}

void ThumbnailList::changeSelected(int i)
{
    Thumbnail *t;
    t = dynamic_cast<Thumbnail *>(cellWidget(m_selected, 0));
    if (t) t -> setSelected(false);
    m_selected = i;
    t = dynamic_cast<Thumbnail *>(cellWidget(m_selected, 0));
    if (t) t -> setSelected(true);
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

void ThumbnailList::setPages(int i, double ar)
{
    Thumbnail *t;
    m_ar = ar;
    setNumRows(i);
    for(int j=1; j <= i; j++)
    {
        t = new Thumbnail(this, QString::number(j), viewport()->paletteBackgroundColor(), (int)(visibleWidth()*ar), visibleWidth());
        setCellWidget(j-1, 0, t);
        setRowHeight(j-1, t->sizeHint().height());
    }
    m_heightLimit = 0;
}

void ThumbnailList::setThumbnail(int i, const QImage *thumbnail)
{
    Thumbnail *t;
    t = dynamic_cast<Thumbnail *>(cellWidget(i-1, 0));
    t->setImage(thumbnail);
}

void ThumbnailList::viewportResizeEvent(QResizeEvent *)
{
    // that if are here to avoid recursive resizing of death
    // where the user makes the window smaller, that makes appear
    // the vertical scrollbar, that makes thumbnails smaller, and
    // while they get smaller the vertical scrollbar is not needed 
    // and ...
    // ... it also works for when the user makes the window larger
    // and then the scrollbar disappears but that makes thumbnails
    // larger and then scrollbar reappears and ...
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
                m_heightLimit = (int) ceil(numRows() * ((visibleWidth() + verticalScrollBar() -> width()) * m_ar + t -> labelSizeHintHeight()));
        }
    }
}

void ThumbnailList::resizeThumbnails()
{
    Thumbnail *t;
    for(int i = 0; i < numRows(); ++i)
    {
        t = dynamic_cast<Thumbnail *>(cellWidget(i, 0));
        t->setImageSize((int)(visibleWidth()*m_ar), visibleWidth());
        setRowHeight(i, (int)(visibleWidth()*m_ar) + t->labelSizeHintHeight());
    }
}

#include "thumbnaillist.moc"
