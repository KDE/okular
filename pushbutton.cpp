/****************************************************************************
**
** An enhanced pushbutton.
**
** Copyright (C) 1997 by Markku Hihnala. 
** This class is freely distributable under the GNU Public License.
**
*****************************************************************************/

#include "pushbutton.h"
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

PushButton::PushButton( QWidget * parent , const char * name )
    : QPushButton( parent, name )
{
}

void PushButton::mousePressEvent ( QMouseEvent *e )
{
	if ( e->button() == LeftButton )
		QPushButton::mousePressEvent( e );
	else if ( e->button() == RightButton )
		emit button3Pressed( mapToGlobal( e->pos() ) );
//	else if ( e->button() == MidButton )
//		emit button2Pressed();
}
