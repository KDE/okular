/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qtextedit.h>
#include <qlabel.h>
#include <kdebug.h>

#include <klistview.h>
#include <qstringlist.h>
#include <qstring.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kurllabel.h>
#include <klocale.h>

#include "gvlogwindow.h"

GSLogWindow::GSLogWindow( const QString& caption, 
                      QWidget* parent, const char* name) :
    QVBox ( parent, name )
{
    kdDebug() << "Starting logwindow" <<endl;
    m_searchLine = new KListViewSearchLine(this);
    m_msgList = new KListView(this);

    QValueList< int > searchCols;
    searchCols.append(0);
    m_searchLine -> setSearchColumns (searchCols);

    m_searchLine -> setListView(m_msgList);
    // width will be fixed later
    m_tCol=m_msgList -> addColumn ("Text",10);
    m_msgList -> addColumn ("InternalType",0);

    connect( this, SIGNAL( user1Clicked() ),this, SLOT( clear() ) );
    connect( &m_clearTimer, SIGNAL(timeout()), this, SLOT(appendBuffered()));
}

bool GSLogWindow::event( QEvent * event )
{
    QVBox ::event(event);
    if ( event->type() == QEvent::Reparent && ( m_msgList->childCount() ) )
    {
        int w=( m_msgList->firstChild() ) -> width(  m_msgList->fontMetrics() , m_msgList, m_tCol);
        kdDebug() << "new width = " << w << endl;
        m_msgList->setColumnWidth(m_tCol, w);
    }
    return true;
}

void GSLogWindow::append( GSInterpreterLib::MessageType t, const QString &text)
{
    //kdDebug() << "Appending: " << text <<endl;
    kdDebug() << "last int: " << m_lastInt << endl;
    QStringList l=QStringList::split("\n",text.stripWhiteSpace());
    QStringList::Iterator it=l.begin(), end=l.end();
    while (it!=end)
    {

    KListViewItem* tmp;
    switch(t)
    {
        case GSInterpreterLib::Error:
            tmp=new KListViewItem( m_msgList , *it, "Error" );
            tmp->setPixmap(m_tCol,SmallIcon( "messagsebox_critical" ));
            break;
        case GSInterpreterLib::Input:
            tmp=new KListViewItem( m_msgList , *it, "Input" );
            tmp->setPixmap(m_tCol,SmallIcon( "1leftarrow" ));
            break;
        case GSInterpreterLib::Output:
            tmp=new KListViewItem( m_msgList , *it, "Output" );
            tmp->setPixmap(m_tCol,SmallIcon( "1rightarrow" ));
            break;
    }
    ++it;
    }
}

void GSLogWindow::append( GSInterpreterLib::MessageType t, const char* buf, int num )
{
    // ghostscript splits messages longer then 128 to chunks, handle this properly
    if (m_lastInt == 128)
    {
        kdDebug() << "last was full line" << endl;
        if (t==m_buffer.first)
        {
            kdDebug() << "appending to buffer" << endl;
            m_buffer.second +=QString::fromLocal8Bit( buf, num );
        }
        else
        {
            kdDebug() << "appending from buffer" << endl;
            // sets m_lastInt to 0
            appendBuffered();
        }
    }

    if (num==128)
    {
        kdDebug() << "this is full line" << endl;
        if (m_lastInt != 128)
        {
            kdDebug() << "appending to buffer" << endl;
            m_buffer.first=t;
            m_buffer.second=QString::fromLocal8Bit( buf, num );
        }
        m_clearTimer.stop();
        m_clearTimer.start(20,FALSE);
    }
    else
    {
        kdDebug() << "this is normal line" << endl;
        if (m_lastInt == 128)
        {
            kdDebug() << "appending from buffer" << endl;
            appendBuffered();
        }
        else
        {
            kdDebug() << "appending directly" << endl;
            append(t,QString::fromLocal8Bit( buf, num ));
            m_clearTimer.stop();
        }
    }
    m_lastInt=num;

    //    kdDebug()<< "LogWindow before split: " << msgString << " lenght" << num << endl;    

}

void GSLogWindow::clear()
{
    m_msgList ->clear();
}

#include "gvlogwindow.moc"
