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
#include "document.h"
#include "page.h"

Thumbnail::Thumbnail( QWidget *parent, const KPDFPage *page )
    : QWidget( parent ), m_page( page ), m_previewWidth( 0 ), m_previewHeight( 0 ), m_selected( false )
{
    m_labelNumber = page->number() + 1;
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
    m_labelHeight = QFontMetrics( font() ).height();
    m_labelWidth = width;

    // resize the widget
    int totalHeight = m_previewHeight + 3 + m_labelHeight;
    resize( width, totalHeight );

    // return this->height plus a little (4px) margin to the next page
    return totalHeight + 4;
}

void Thumbnail::setSelected( bool selected )
{
    m_selected = selected;
    update( 0, m_previewHeight + 3, m_labelWidth, m_labelHeight );
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

    // draw the bottom label
    QColor fillColor = m_selected ? palette().active().highlight() : palette().active().base();
    p.fillRect( 0, m_previewHeight + 3, m_labelWidth, m_labelHeight, fillColor );
    p.drawText( 0, m_previewHeight + 3, m_labelWidth, m_labelHeight, Qt::AlignCenter, QString::number( m_labelNumber ) );

    // draw page outline
    p.drawRect( 0, 0, m_previewWidth + 2, m_previewHeight + 2 );
    p.setPen( Qt::gray );
    p.drawLine( 4, m_previewHeight + 2, m_previewWidth + 2, m_previewHeight + 2 );
    p.drawLine( m_previewWidth + 2, 4, m_previewWidth + 2, m_previewHeight + 2 );

    // draw the pixmap
    p.translate( 1, 1 );
    clipRect.moveBy( -1, -1 );
    clipRect = clipRect.intersect( QRect( 0, 0, m_previewWidth, m_previewHeight ) );
    m_page->drawPixmap( THUMBNAILS_ID, &p, clipRect, m_previewWidth, m_previewHeight );

    p.end();
}
