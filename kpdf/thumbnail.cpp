/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qapplication.h>
#include <qimage.h>
#include <qlabel.h>
#include <qpixmap.h>

#include "thumbnail.h"
#include "page.h"

Thumbnail::Thumbnail(QWidget *parent, const QColor &color, const KPDFPage *page) : QVBox(parent), m_page(page), m_backgroundColor(color)
{
    m_thumbnailW = new QWidget(this);
    m_thumbnailW->setEraseColor(Qt::gray);
    m_label = new QLabel(QString::number(page->number()+1), this);
    m_label->setAlignment(AlignCenter);
    setPaletteBackgroundColor(m_backgroundColor);
}

int Thumbnail::pageNumber()
{
    return m_page->number();
}

void Thumbnail::setImage(const QImage *thumbnail)
{
    // TODO i am almost sure this can be done inside the thumbnailgenerator thread
    m_original = thumbnail->smoothScale(m_thumbnailW->size());
    m_thumbnailW->setPaletteBackgroundPixmap(m_original);
}

int Thumbnail::setThumbnailWidth(int width)
{
    int height = (int)(m_page->ratio() * width);
    m_thumbnailW->setFixedHeight(height);
    m_thumbnailW->setFixedWidth(width);
    height += m_label->sizeHint().height();
    resize( width, height );
    return height;
}

void Thumbnail::setSelected(bool selected)
{
    if (selected) setPaletteBackgroundColor(QApplication::palette().active().highlight());
    else setPaletteBackgroundColor(m_backgroundColor);
}


#include "thumbnail.moc"
