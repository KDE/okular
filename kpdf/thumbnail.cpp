/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qlabel.h>
#include <qpainter.h>

#include "thumbnail.h"
#include "page.h"

Thumbnail::Thumbnail( QWidget *parent, const KPDFPage *page )
    : QWidget( parent ), m_page( page ), m_previewWidth( 0 ), m_previewHeight( 0 )
{
    m_label = new QLabel( QString::number( page->number() + 1 ), this );
    m_label->setAlignment( AlignCenter );
    setPaletteBackgroundColor( palette().active().base() );
}

//BEGIN commands 
int Thumbnail::setThumbnailWidth( int width )
{
    // compute and update drawable area dimensions
    // note: 3 pixels are left (in each dimension) for page decorations 
    m_previewWidth = width - 3;
    //TODO albert: check page rotation for aspect ratio
    m_previewHeight = (int)(m_page->ratio() * width);

    // reposition label at bottom
    int labelHeight = m_label->sizeHint().height();
    m_label->setGeometry( 0, m_previewHeight + 3, width, labelHeight );

    // resize the widget
    int totalHeight = m_previewHeight + 3 + labelHeight;
    resize( width, totalHeight );

    // return this->height plus a little (4px) margin to the next page
    return totalHeight + 4;
}

void Thumbnail::setSelected( bool selected )
{
    // alternate 'base' or 'hilight' colors to represent selection
    if (selected) m_label->setPaletteBackgroundColor( palette().active().highlight() );
    else m_label->setPaletteBackgroundColor( palette().active().base() );
}
//END commands 

//BEGIN query methods 
int Thumbnail::pageNumber() const
{
    return m_page->number();
}

int Thumbnail::previewWidth() const
{
    return m_previewWidth;
}

int Thumbnail::previewHeight() const
{
    return m_previewHeight;
}
//END query methods 

void Thumbnail::paintEvent( QPaintEvent * e )
{
    QRect clipRect = e->rect();
    QPainter p( this );

    // draw page outline
    p.setPen( Qt::black );
    p.drawRect( 0, 0, m_previewWidth + 2, m_previewHeight + 2 );
    p.setPen( Qt::gray );
    p.drawLine( 4, m_previewHeight + 2, m_previewWidth + 2, m_previewHeight + 2 );
    p.drawLine( m_previewWidth + 2, 4, m_previewWidth + 2, m_previewHeight + 2 );

    // draw the pixmap
    p.translate( 1, 1 );
    clipRect.moveBy( -1, -1 );
    clipRect = clipRect.intersect( QRect( 0, 0, m_previewWidth, m_previewHeight ) );
    m_page->drawThumbnail( &p, clipRect, m_previewWidth, m_previewHeight );

    p.end();
}
