/****************************************************************************
**
** A simple widget to control scrolling in two dimensions.
**
** Copyright (C) 1997 by Markku Hihnala. 
** This class is freely distributable under the GNU Public License.
**
*****************************************************************************/


#include <qdrawutil.h>
#include <qpainter.h>

#include "scrbox.h"

ScrollBox::ScrollBox( QWidget * parent , const char * name )
    : QFrame( parent, name )
{
	setFrameStyle( Panel | Sunken );
	setBackgroundMode();
}

void ScrollBox::mousePressEvent ( QMouseEvent *e )
{
	mouse = e->pos();
	if ( e->button() == RightButton )
		emit button3Pressed();
	if ( e->button() == MidButton )
		emit button2Pressed();
}

void ScrollBox::mouseMoveEvent ( QMouseEvent *e )
{
	if (e->state() != LeftButton)
		return;
	int dx = ( e->pos().x() - mouse.x() ) *
		 pagesize.width() /
		 width();
	int dy = ( e->pos().y() - mouse.y() ) * 
		 pagesize.height() /
		 height();
	
        // Notify the word what the view position has changed
        // The word in turn will notify as that view position has changed
        // Even if coordinates are out of range JScrollView hendle
        // this properly
        emit valueChanged( viewpos + QPoint( dx, dy ) );

	mouse = e->pos();
}

void ScrollBox::drawContents ( QPainter *p )
{
        if (pagesize.isNull())
		return;

	QRect c( contentsRect() );

	int len = pagesize.width();
	int x = c.x() + c.width() * viewpos.x() / len;
	int w = c.width() * viewsize.width() / len ;
	if ( w > c.width() ) w = c.width();

	len = pagesize.height();
	int y = c.y() + c.height() * viewpos.y() / len;
	int h = c.height() * viewsize.height() / len;
	if ( h > c.height() ) h = c.height();

	QBrush b( colorGroup().background() );
        qDrawShadePanel( p, x, y, w, h, colorGroup(), FALSE, 1, &b );
}

void ScrollBox::setPageSize( QSize s )
{
	pagesize = s;
	setBackgroundMode();
	repaint();
}

void ScrollBox::setViewSize( QSize s )
{
	viewsize = s;
	setBackgroundMode();
	repaint();
}

void ScrollBox::setViewPos( QPoint pos )
{
        viewpos = pos;
        repaint();
}



void ScrollBox::setBackgroundMode()
{
  if( pagesize.isNull() || 
      (viewsize.rwidth() >= pagesize.rwidth() && 
       viewsize.rheight() >= pagesize.rheight()) )
  {
    QFrame::setBackgroundMode( PaletteBackground );
  }
  else
  {
    QFrame::setBackgroundMode( PaletteMid ); // As in QScrollBar
  }
}



