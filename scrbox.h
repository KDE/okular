/****************************************************************************
**
** A simple widget to control scrolling in two dimensions.
**
** Copyright (C) 1997 by Markku Hihnala. 
** This class is freely distributable under the GNU Public License.
**
*****************************************************************************/

#ifndef SCROLL_BOX_H
#define SCROLL_BOX_H

#include <qframe.h>
#include <qrangect.h>

class ScrollBox: public QFrame
{
	Q_OBJECT

public:
	ScrollBox( QWidget * parent = 0, const char * name = 0 );

public slots:
	void	setPageSize( QSize );
	void	setViewSize( QSize );
	void	setViewPos( QPoint );

signals:
	void	valueChanged( QPoint );
	void	button2Pressed(); 
	void	button3Pressed(); 

protected:
	void	mousePressEvent ( QMouseEvent *);
	void	mouseMoveEvent ( QMouseEvent *);
	void	drawContents ( QPainter *);

private:
	QPoint	viewpos, mouse;
	QSize	pagesize;
	QSize	viewsize;
};

#endif
