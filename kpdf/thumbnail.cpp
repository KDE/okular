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

Thumbnail::Thumbnail(QWidget *parent, const QString &text, const QColor &color, int height, int width) : QVBox(parent), m_backgroundColor(color)
{
    m_thumbnailW = new QWidget(this);
    m_thumbnailW->setFixedHeight(height);
    m_thumbnailW->setFixedWidth(width);
    m_label = new QLabel(text, this);
    m_label->setAlignment(AlignCenter);
    setPaletteBackgroundColor(m_backgroundColor);
}

void Thumbnail::setPixmap(const QPixmap *thumbnail)
{
    m_original = thumbnail->convertToImage().smoothScale(m_thumbnailW->size());
    m_thumbnailW->setPaletteBackgroundPixmap(m_original);
}

void Thumbnail::setPixmapSize(int height, int width)
{
    m_thumbnailW->setFixedHeight(height);
    m_thumbnailW->setFixedWidth(width);
}

int Thumbnail::getPixmapHeight() const
{
    return m_thumbnailW->size().height();
}

void Thumbnail::resizeEvent(QResizeEvent *)
{
    QImage im;
    if (!m_original.isNull())
    {
        im = m_original.smoothScale(m_thumbnailW->size());
        m_thumbnailW->setPaletteBackgroundPixmap(im);
    }
}

void Thumbnail::setSelected(bool selected)
{
    if (selected) setPaletteBackgroundColor(QApplication::palette().active().highlight());
    else setPaletteBackgroundColor(m_backgroundColor);
}

int Thumbnail::labelSizeHintHeight()
{
    return m_label->sizeHint().height();
}


#include "thumbnail.moc"
