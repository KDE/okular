/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qpainter.h>
#include <qsize.h>

#include "pixmapwidget.h"
#include "document.h"
#include "page.h"

/** PixmapWidget TODO: check image rotation for aspect ratio (and implement rotation) **/

PixmapWidget::PixmapWidget( QWidget * parent, const KPDFPage * kp )
    : QWidget( parent, 0, WNoAutoErase ), m_page( kp ),
    m_marginLeft(0), m_marginTop(0), m_marginRight(0), m_marginBottom(0),
    m_pixmapWidth(0), m_pixmapHeight(0), m_zoomFactor( 1.0 )
{
}

void PixmapWidget::setZoomFixed( double magFactor )
{
    m_pixmapWidth = (int)( magFactor * m_page->width() );
    m_pixmapHeight = (int)( magFactor * m_page->height() );
    m_zoomFactor = magFactor;

    // resize the widget FIXME REMOVE THIS; TEMPORARY, resize from the outside
    resize( m_marginLeft + m_pixmapWidth + m_marginRight,
            m_marginTop + m_pixmapHeight + m_marginBottom );
}

void PixmapWidget::setZoomFitWidth( int width )
{
    m_pixmapWidth = width - m_marginLeft - m_marginRight;
    m_pixmapHeight = (int)(m_page->ratio() * m_pixmapWidth);
    // compute equivalent zoom factor
    m_zoomFactor = m_page->width() / (float)m_pixmapWidth;

    // resize the widget FIXME REMOVE THIS; TEMPORARY, resize from the outside
    resize( width, m_marginTop + m_pixmapHeight + m_marginBottom );
}

void PixmapWidget::setZoomFitRect( int rectWidth, int rectHeight )
{
    rectWidth -= (m_marginLeft + m_marginRight);
    rectHeight -= (m_marginTop + m_marginBottom);
    double scaleW = (double)rectWidth / (double)m_page->width();
    double scaleH = (double)rectHeight / (double)m_page->height();
    setZoomFixed( scaleW < scaleH ? scaleW : scaleH );
}

QSize PixmapWidget::sizeHint() const
{
    return QSize( m_marginLeft + m_pixmapWidth + m_marginRight,
                  m_marginTop + m_pixmapHeight + m_marginBottom );
}

int PixmapWidget::widthHint() const
{
    return m_marginLeft + m_pixmapWidth + m_marginRight;
}

int PixmapWidget::heightHint() const
{
    return m_marginTop + m_pixmapHeight + m_marginBottom;
}

QSize PixmapWidget::pixmapSize() const
{
    return QSize( m_pixmapWidth, m_pixmapHeight );
}

int PixmapWidget::pageNumber() const
{
    return m_page->number();
}

void PixmapWidget::setPixmapMargins( int left, int top, int right, int bottom )
{
    m_marginLeft = left;
    m_marginTop = top;
    m_marginRight = right;
    m_marginBottom = bottom;
}


/** ThumbnailWidget **/

ThumbnailWidget::ThumbnailWidget( QWidget *parent, const KPDFPage *page )
    : PixmapWidget( parent, page ), m_selected( false )
{
    m_labelNumber = page->number() + 1;
    m_labelHeight = QFontMetrics( font() ).height();
    setPixmapMargins( 2, 1, 2, m_labelHeight + 2 );
    setPaletteBackgroundColor( palette().active().base() );
}

void ThumbnailWidget::setSelected( bool selected )
{
    m_selected = selected;
    update( 0, m_pixmapHeight + 3, width(), m_labelHeight );
}

void ThumbnailWidget::paintEvent( QPaintEvent * e )
{
    int width = m_marginLeft + m_pixmapWidth + m_marginRight;
    QRect clipRect = e->rect();
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
        m_page->drawPixmap( THUMBNAILS_ID, &p, clipRect, m_pixmapWidth, m_pixmapHeight );
    }

    p.end();
}


/** PageWidget **/

PageWidget::PageWidget( QWidget *parent, const KPDFPage *page )
    : PixmapWidget( parent, page )
{
    //
    //
    setPixmapMargins( 1,1,3,3 );
}

void PageWidget::paintEvent( QPaintEvent * e )
{
    QColor bc( paletteBackgroundColor() /*KGlobalSettings::calculateAlternateBackgroundColor( KGlobalSettings::baseColor() )*/ );

    QRect clip = e->rect();
    QRect pageClip = clip.intersect( QRect( m_marginLeft, m_marginTop, m_pixmapWidth, m_pixmapHeight ) );

    QPainter p( this );

    // if drawn region includes an edge of the page
    if ( pageClip != clip )
    {
        // draw the outline and adapt pageClip
        p.setPen( Qt::black );
        p.drawRect( QRect( 0,0, m_pixmapWidth + 2, m_pixmapHeight + 2 ) );
        p.setPen( Qt::gray );
        /*
        p.drawLine( pageClip.left(), pageClip.bottom() + 1,  pageClip.right() + 1, pageClip.bottom() + 1 );
        p.drawLine( pageClip.right() + 1, pageClip.top(), pageClip.right() + 1, pageClip.bottom() + 1 );
        p.setPen( Qt::lightGray );
        p.drawLine( pageClip.left(), pageClip.bottom() + 2,  pageClip.right() + 2, pageClip.bottom() + 2 );
        p.drawLine( pageClip.right() + 2, pageClip.top(), pageClip.right() + 2, pageClip.bottom() + 2 );
        p.setPen( bc );
        p.drawPoint( pageClip.left(), pageClip.bottom() + 2 );
        p.drawPoint( pageClip.right() + 2, pageClip.top() );
        pageClip.setWidth( pageClip.width() + 2 );
        pageClip.setHeight( pageClip.height() + 2 );
        */
    }

    // draw page (inside pageClip rectangle)
    p.translate( m_marginLeft, m_marginTop );
    pageClip.moveBy( -m_marginLeft, -m_marginTop );
    m_page->drawPixmap( PAGEVIEW_ID, &p, pageClip, m_pixmapWidth, m_pixmapHeight );
    p.translate( -m_marginLeft, -m_marginTop );

    p.end();
}
