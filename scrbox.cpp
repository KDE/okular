/****************************************************************************
**
** A simple widget to control scrolling in two dimensions.
**
** Copyright (C) 1997 by Markku Hihnala. 
** This class is freely distributable under the GNU Public License.
**
*****************************************************************************/

#include "scrbox.h"
#include <qtabdlg.h>
#include <qmlined.h>
#include <qbttngrp.h>
#include <qradiobt.h>
#include <qlabel.h>
#include <qcombo.h>
#include <qlayout.h>
#include <stdio.h>
#include <qstring.h>
#include <qpainter.h>
#include <qdrawutl.h>

ScrollBox::ScrollBox( QWidget * parent , const char * name )
    : QFrame( parent, name )
{
	setFrameStyle( Panel | Sunken );
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
	
	setViewPos( viewpos + QPoint( dx, dy ) );
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

	qDrawShadePanel( p, x, y, w, h, colorGroup(), FALSE, 1, 0);
}

void ScrollBox::setPageSize( QSize s )
{
	pagesize = s;
	update();
}

void ScrollBox::setViewSize( QSize s )
{
	viewsize = s;
	update();
}

void ScrollBox::setViewPos( QPoint pos )
{
	QPoint old = viewpos;
	int x = pos.x();
	int y = pos.y();
	QSize limit = pagesize - viewsize + QSize(1,1);
	if ( x > limit.width() ) x = limit.width();
	if ( x < 0 ) x = 0;
	if ( y > limit.height() ) y = limit.height();
	if ( y < 0 ) y = 0;

	viewpos = QPoint( x, y );

	if ( viewpos == old )
		return;
	emit valueChanged( viewpos );
	update();
}
