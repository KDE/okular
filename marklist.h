/****************************************************************************
**
** A simple widget to mark and select entries in a list.
**
** Copyright (C) 1997 by Markku Hihnala. 
** This class is freely distributable under the GNU Public License.
**
*****************************************************************************/

#ifndef MARKLIST_H
#define MARKLIST_H

#include <qtablevw.h>
#include "qpopmenu.h"
#include <qstrlist.h>

class MarkListItem
{
    public:
        MarkListItem( const char *s ) : marked(0), selected(0)  { _text = s; }
	void	setMark( bool flag )	{ marked = flag; }
	bool	mark()			{ return marked; }
	void	setSelect( bool flag )	{ selected = flag; }
	bool	select()		{ return selected; }
	const char *text()		{ return _text.data(); }

    private:
	bool	marked;
	bool	selected;
	QString _text;
}; 

class MarkList: public QTableView
{
	Q_OBJECT

public:
	MarkList( QWidget * parent = 0, const char * name = 0 );
	~MarkList() { }
	QStrList *	markList();
	void	insertItem ( const char *text, int index=-1);
	int	count();
	void	setAutoUpdate ( bool enable );
	void	clear();

public slots:
	void	select(int);
	void	markSelected();
	void	unmarkSelected();

signals:
	void	selected( int index );
	void	selected( const char * text );

protected:
	void	mousePressEvent ( QMouseEvent* );
	void	mouseReleaseEvent ( QMouseEvent* ) {}
	void	mouseMoveEvent ( QMouseEvent* );
	void	paintCell( QPainter *p, int row, int col );
	int	cellWidth( int );
	void	updateItem( int i );

protected slots:
	void	markAll();
	void	markEven();
	void	markOdd();
	void	toggleMarks();
	void	removeMarks();

private:
	void	changeMarks( int, int = 0 );
	QPoint	mouse;
	int	sel;
	QPopupMenu* pup;
	int	drag;
	QList<MarkListItem> items;
};

#endif 
