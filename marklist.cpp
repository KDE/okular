/****************************************************************************
**
** A simple widget to mark and select entries in a list.
**
** Copyright (C) 1997 by Markku Hihnala. 
** This class is freely distributable under the GNU Public License.
**
*****************************************************************************/

#include "marklist.h"
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
#include <kapp.h>
#include <qpopmenu.h>
#include <klocale.h>
 
MarkList::MarkList( QWidget * parent , const char * name )
	: QTableView( parent, name ), sel(-1), drag(-1), items()
{
	setFrameStyle( Panel | Sunken );
        setTableFlags( Tbl_autoVScrollBar | Tbl_cutCellsV
                         | Tbl_snapToVGrid | Tbl_clipCellPainting);
        setLineWidth( 2 );
        setCellHeight( fontMetrics().lineSpacing() );
   	setNumCols( 2 );
	pup = new QPopupMenu;
	pup->setMouseTracking(TRUE);
	pup->insertItem( i18n("Mark current"), this, SLOT(markSelected()) );
	pup->insertItem( i18n("Unmark current"), this, SLOT(unmarkSelected()) );
	pup->insertItem( i18n("Mark all"), this, SLOT(markAll()) );
	pup->insertItem( i18n("Mark even"), this, SLOT(markEven()) );
	pup->insertItem( i18n("Mark odd"), this, SLOT(markOdd()) );
	pup->insertItem( i18n("Toggle marks"), this, SLOT(toggleMarks()) );
	pup->insertItem( i18n("Remove marks"), this, SLOT(removeMarks()) );
}

void	MarkList::insertItem ( const char *text, int index)
{
	MarkListItem *mli = new MarkListItem( text );
	items.insert( index, mli );
	setNumRows( items.count() );
}

int MarkList::count()
{
	return items.count();
}

void	MarkList::setAutoUpdate ( bool enable)
{
	QTableView::setAutoUpdate( enable );
	if (enable) update();
}

void	MarkList::clear()
{
	items.clear();
	sel = -1;
	update();
}

int	MarkList::cellWidth( int col )
{
	return col==0 ? 16 : width()-16-2*frameWidth();
}

void MarkList::paintCell( QPainter *p, int row, int col)
{
	if ( col == 0 && items.at( row )->mark() )
	{
		p->setBrush( red );
		p->drawEllipse( 5, 4, 6, 6 );
	}

	if ( col == 1 )
	{
		if ( items.at( row )->select() )
		{
			QColorGroup cg = QApplication::palette()->normal();
                        QBrush tmpB(colorGroup().light());
			qDrawShadePanel( p, 0, 0, cellWidth( 1 ) - 1, cellHeight(),
				cg, FALSE, 1, &tmpB);
		}

		QFontMetrics fm = p->fontMetrics();
		int yPos;   // vertical text position
		if ( 10 < fm.height() )
			yPos = fm.ascent() + fm.leading()/2;
		else
			yPos = 5 - fm.height()/2 + fm.ascent();
		p->drawText( 4, yPos, items.at( row )->text() );
	}
}

void MarkList::mousePressEvent ( QMouseEvent *e )
{
	int i = findRow( e->pos().y() );
	if ( i == -1 )
		return;
	MarkListItem *it = items.at( i );

	if ( e->button() == LeftButton )
		select( i );
	else if ( e->button() == RightButton )
		pup->popup( mapToGlobal( e->pos() ) );
	else if ( e->button() == MidButton )
	{
		it->setMark( !it->mark() );
		updateCell( i, 0 );
		drag = i;
	}
}

void MarkList::mouseMoveEvent ( QMouseEvent *e )
{
	if (e->state() != MidButton)
		return;

	int i = findRow( e->pos().y() );
	if ( i == drag || i == -1 )
		return;
	do {
		drag += i > drag ? 1 : -1;
		items.at( drag )->setMark( !items.at( drag )->mark() );
		updateCell( drag, 0 );
	} while ( i != drag );
}

void MarkList::select( int i )
{
	if ( i < 0 || i >= int(items.count()) || i == sel )
		return;
	MarkListItem *it = items.at( i );
	if ( sel != -1 )
	{
		items.at( sel )->setSelect( FALSE );
		updateCell( sel, 1 );
	}
	it->setSelect( TRUE );
	sel = i;
	updateCell( i, 1 );
	emit selected( i );
	emit selected( it->text() );
	if ( ( i<=0 || rowIsVisible( i-1 ) ) &&
	     ( i>=int(items.count())-1 || rowIsVisible( i+1 ) ) )
		return;
	setTopCell( QMAX( 0, i - viewHeight()/cellHeight()/2 ) );
}

void MarkList::markSelected()
{
	if ( sel == -1 )
		return;
	MarkListItem *it = items.at( sel );
	it->setMark( TRUE );
	updateCell( sel, 0 );
}

void MarkList::unmarkSelected()
{
	if ( sel == -1 )
		return;
	MarkListItem *it = items.at( sel );
	it->setMark( FALSE );
	updateCell( sel, 0 );
}

void MarkList::markAll()
{
	changeMarks( 1 );
}

void MarkList::markEven()
{
	changeMarks( 1, 2 );	
}

void MarkList::markOdd()
{
	changeMarks( 1, 1 );
}


void MarkList::removeMarks()
{
	changeMarks( 0 );
}

void MarkList::toggleMarks()
{
	changeMarks( 2 );
}

void MarkList::changeMarks( int how, int which  )
{
	MarkListItem *it;
	QString t;

	setUpdatesEnabled( FALSE );
	for ( int i=items.count(); i-->0 ; )
	{
		if ( which )
		{
			t = items.at( i )->text();
			if ( t.toInt() % 2 == which - 1 )
				continue;
		}
		it = items.at( i );
		if ( how == 2 )
			it->setMark( ! it->mark() );
		else	it->setMark( how );
		updateCell( i, 0 );
	}
	setUpdatesEnabled( TRUE );
}

QStrList *MarkList::markList()
{
	QStrList *l = new QStrList;

	for ( unsigned i=0 ; i < items.count(); i++ )
		if ( ( items.at( i ) )->mark() )
			l->append( items.at( i )->text() );
	return l;
}
