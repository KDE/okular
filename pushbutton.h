/****************************************************************************
**
** An enhanced pushbutton.
**
** Copyright (C) 1997 by Markku Hihnala. 
** This class is freely distributable under the GNU Public License.
**
*****************************************************************************/

#ifndef PUSHBUTTON_H
#define PUSHBUTTON_H

#include <qpushbt.h>
#include <qpopmenu.h>

class PushButton: public QPushButton
{
	Q_OBJECT

public:
	PushButton( QWidget * parent = 0, const char * name = 0 );

signals:
	void	button3Pressed( QPoint ); 

protected:
	void	mousePressEvent ( QMouseEvent *);

private:
	QPopupMenu* menu;
};

#endif
