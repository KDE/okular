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
#include <kglobalsettings.h>
#include <kdebug.h>

#include "pixmapwidget.h"
#include "document.h"
#include "page.h"
#include "settings.h"

// TODO : REMOVE "#ifndef NDEBUG" stuff before merging to HEAD! (added 20041024)

PixmapWidget::PixmapWidget( QWidget * parent, const KPDFPage * kp, const char * name )
/* Note on PixmapWidget Wflags (Enrico):
    It's SO IMPORTANT to set WNoAutoErase and WStaticContents.
    If not, already exposed widgets will receive a paint event when resized
    (for example when zooming/relayouting in PageView) even if not shown in
    current PageView's viewport.
*/
    : QWidget( parent, name, WNoAutoErase | WStaticContents ), m_page( kp ),
    m_marginLeft( 0 ), m_marginTop( 0 ), m_marginRight( 0 ), m_marginBottom( 0 ),
    m_pixmapWidth( -1 ), m_pixmapHeight( -1 ),
    m_zoomFactor( 1.0 )
{
}

void PixmapWidget::setZoomFixed( double magFactor )
{
    m_pixmapWidth = (int)( magFactor * m_page->width() );
    m_pixmapHeight = (int)( magFactor * m_page->height() );
    m_zoomFactor = magFactor;
}

void PixmapWidget::setZoomFitWidth( int width )
{
    m_pixmapWidth = width - m_marginLeft - m_marginRight;
    m_pixmapHeight = (int)(m_page->ratio() * m_pixmapWidth);
    // compute equivalent zoom factor
    m_zoomFactor = (double)m_pixmapWidth / (double)m_page->width();
}

void PixmapWidget::setZoomFitRect( int rectWidth, int rectHeight )
{
    rectWidth -= (m_marginLeft + m_marginRight);
    rectHeight -= (m_marginTop + m_marginBottom);
    double scaleW = (double)rectWidth / (double)m_page->width();
    double scaleH = (double)rectHeight / (double)m_page->height();
    setZoomFixed( scaleW < scaleH ? scaleW : scaleH );
}

int PixmapWidget::widthHint() const
{
#ifndef NDEBUG
    return m_marginLeft + pixmapWidth() + m_marginRight;
#else
    return m_marginLeft + m_pixmapWidth + m_marginRight;
#endif
}

int PixmapWidget::heightHint() const
{
#ifndef NDEBUG
    return m_marginTop + pixmapHeight() + m_marginBottom;
#else
    return m_marginTop + m_pixmapHeight + m_marginBottom;
#endif
}

int PixmapWidget::pixmapWidth() const
{
#ifndef NDEBUG
    // code enabled for dev/testers only. please make sure that setZoom*
    // gets called
    if ( m_pixmapWidth < 0 )
    {
	kdDebug() << "No pixmapWidth set for page " << m_page->number() << " !" << endl;
	return 1;
    }
#endif
    return m_pixmapWidth;
}

int PixmapWidget::pixmapHeight() const
{
#ifndef NDEBUG
    // code enabled for dev/testers only. please make sure that setZoom*
    // gets called
    if ( m_pixmapHeight < 0 )
    {
	kdDebug() << "No pixmapHeight set for page " << m_page->number() << " !" << endl;
	return 1;
    }
#endif
    return m_pixmapHeight;
}

int PixmapWidget::pageNumber() const
{
    return m_page->number();
}

const KPDFPage * PixmapWidget::page() const
{
    return m_page;
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
    : PixmapWidget( parent, page, "pageWidget" ), m_selBeginX( -1 ), m_selBeginY( -1 )
{
    // keep bottom equal to right margin
    setPixmapMargins( 1, 1, 4, 4 );
}

void PageWidget::clearSelection()
{
    if ( !m_selectionRect.isNull() )
        update( m_selectionRect );
    m_selBeginX = -1;
    m_selBeginY = -1;
    m_selectionRect = QRect();
}

void PageWidget::setBeginCorner( int x, int y )
{
    if ( !m_selectionRect.isNull() )
        update( m_selectionRect );
    m_selBeginX = x;
    m_selBeginY = y;
    m_selectionRect.setCoords( x, y, 1, 1 );
    update( m_selectionRect );
}

void PageWidget::setEndCorner( int x, int y )
{
    QRect newRect( m_selBeginX, m_selBeginY, x - m_selBeginX, y - m_selBeginY );
    newRect = newRect.normalize().intersect( QRect( m_marginLeft, m_marginTop, m_pixmapWidth, m_pixmapHeight ) );
    if ( newRect != m_selectionRect )
    {
        if ( !m_selectionRect.isNull() )
            update( m_selectionRect );
        m_selectionRect = newRect;
        update( m_selectionRect );
    }
}

QString PageWidget::selectedText() const
{
    return m_page->getTextInRect( m_selectionRect );
}

void PageWidget::paintEvent( QPaintEvent * e )
{
    QRect clip = e->rect();
    QRect pageClip = clip.intersect( QRect( m_marginLeft, m_marginTop, m_pixmapWidth, m_pixmapHeight ) );

    // FIXME: this prevents outline-only paints (but saves pixmap repaints!!)
    if ( !pageClip.isValid() || pageClip.width() < 1 || pageClip.height() < 1 )
        return;

    // kdDebug() << "repaint on page " << pageNumber() << " on rect: " << clip << endl;

    QPainter p( this );

    // if drawn region includes an edge of the page
    if ( pageClip != clip && m_pixmapWidth > 10 && m_pixmapWidth > 10 )
    {
        // draw the black outline
        p.drawRect( QRect( 0,0, m_pixmapWidth + 2, m_pixmapHeight + 2 ) );
        // draws shadow on left and bottom edges
        int levels = m_marginBottom - 1;
        p.fillRect( 0, 2 + m_pixmapHeight, levels, levels, Qt::gray );
        p.fillRect( 2 + m_pixmapWidth, 0, levels, levels, Qt::gray );
        int r = Qt::gray.red() / (levels + 2),
            g = Qt::gray.green() / (levels + 2),
            b = Qt::gray.blue() / (levels + 2);
        for ( int i = 0; i < levels; i++ )
        {
            p.setPen( QColor( r * (i+2), g * (i+2), b * (i+2) ) );
            p.drawLine( 1 + i, m_pixmapHeight+2 + i,  m_pixmapWidth+2 + i, m_pixmapHeight+2 + i );
            p.drawLine( m_pixmapWidth+2 + i, 1 + i,  m_pixmapWidth+2 + i, m_pixmapHeight+2 + i );
        }
    }

    // draw page (inside pageClip rectangle)
    p.translate( m_marginLeft, m_marginTop );
    pageClip.moveBy( -m_marginLeft, -m_marginTop );

    // accessibility setting
    if ( Settings::renderMode() == Settings::EnumRenderMode::Inverted )
        p.setRasterOp( Qt::NotCopyROP );

    // draw the pixmap
    m_page->drawPixmap( PAGEVIEW_ID, &p, pageClip, m_pixmapWidth, m_pixmapHeight );
#if 0
    // TODO: overlays <ONLY FOR FUN> :-)
    #include <qpixmap.h>
    #include <kstandarddirs.h>
    if ( m_pixmapWidth > 64 && m_pixmapHeight > 64 )
    {
        QPixmap page( locate("data","kpdf/bookmark_corner.png") );
        p.drawPixmap( m_pixmapWidth - page.width(), 0, page );
    }
#endif
    p.translate( -m_marginLeft, -m_marginTop );

    // draw selection rect (uses opaque selection)
    if ( !m_selectionRect.isNull() )
    {
        p.setPen( palette().active().highlight().dark(110) );
        //COOL p.setBrush( QBrush( palette().active().highlight(), Qt::Dense4Pattern ) );
        p.drawRect( m_selectionRect );
    }

    p.end();
}
