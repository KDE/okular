/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid                               *
 *   tsdgeos@terra.es                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "thumbnaillist.h"
#include "thumbnail.h"

ThumbnailList::ThumbnailList(QWidget *parent) : QTable(parent)
{
    setNumCols(1);
    setLeftMargin(0);
    setTopMargin(0);
    setHScrollBarMode(QScrollView::AlwaysOff);
    m_selected = 0;
    
    connect(this, SIGNAL(pressed(int, int, int, const QPoint&)), this, SIGNAL(clicked(int)));
    connect(this, SIGNAL(currentChanged(int, int)), this, SLOT(changeSelected(int)));
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

int ThumbnailList::getCurrentItem() const
{
    return currentRow();
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

void ThumbnailList::setThumbnail(int i, const QPixmap *thumbnail)
{
    Thumbnail *t;
    t = dynamic_cast<Thumbnail *>(cellWidget(i-1, 0));
    t->setPixmap(thumbnail);
}

void ThumbnailList::viewportResizeEvent(QResizeEvent *)
{
    // that if are here to avoid recursive resizing of death
    // where the user makes the window smaller, that makes appear
    // the vertical scrollbar, that makes thumbnails smaller, and
    // while they get smaller the vertical scrollbar is not needed 
    // and....
    Thumbnail *t;
    if (numRows() == 0) return;
    if (size().height() <= m_heightLimit)
    {
        t = dynamic_cast<Thumbnail *>(cellWidget(0, 0));
        if (t->getPixmapHeight() > (int)(visibleWidth()*m_ar))
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
            if (m_heightLimit == 0 && verticalScrollBar() -> isVisible())
                m_heightLimit = size().height();
        }
    }
}

void ThumbnailList::resizeThumbnails()
{
    Thumbnail *t;
    for(int i = 0; i < numRows(); ++i)
    {
        t = dynamic_cast<Thumbnail *>(cellWidget(i, 0));
        t->setPixmapSize((int)(visibleWidth()*m_ar), visibleWidth());
        setRowHeight(i, (int)(visibleWidth()*m_ar) + t->labelSizeHintHeight());
    }
}

#include "thumbnaillist.moc"
