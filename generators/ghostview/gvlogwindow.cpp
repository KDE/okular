/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qlist.h>
#include <qtextedit.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <kdebug.h>

#include <k3listview.h>
#include <qstringlist.h>
#include <qstring.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <k3listviewsearchline.h>
#include <klocale.h>

#include "gvlogwindow.h"

GSLogWindow::GSLogWindow( QWidget* parent )
 : KVBox( parent )
{
    kDebug() << "Starting logwindow" <<endl;
    m_searchLine = new K3ListViewSearchLine(this);
    m_msgList = new K3ListView(this);

    QList< int > searchCols;
    searchCols.append(0);
    m_searchLine -> setSearchColumns (searchCols);

    m_searchLine -> setListView(m_msgList);
    // width will be fixed later
    m_tCol=m_msgList -> addColumn ("Text",10);
    m_msgList -> addColumn ("InternalType",0);

    m_clearTimer.setSingleShot( false );
    connect( &m_clearTimer, SIGNAL(timeout()), this, SLOT(appendBuffered()));
}

bool GSLogWindow::event( QEvent * event )
{
    KVBox::event(event);
    if ( event->type() == QEvent::Reparent && ( m_msgList->childCount() ) )
    {
        int w=( m_msgList->firstChild() ) -> width(  m_msgList->fontMetrics() , m_msgList, m_tCol);
        kDebug() << "new width = " << w << endl;
        m_msgList->setColumnWidth(m_tCol, w);
    }
    return true;
}

void GSLogWindow::append( GSInterpreterLib::MessageType t, const QString &text)
{
    //kDebug() << "Appending: " << text <<endl;
    kDebug() << "last int: " << m_lastInt << endl;
    QStringList l=text.trimmed().split("\n",QString::SkipEmptyParts);
    QStringList::Iterator it=l.begin(), end=l.end();
    while (it!=end)
    {

    K3ListViewItem* tmp;
    switch(t)
    {
        case GSInterpreterLib::Error:
            tmp=new K3ListViewItem( m_msgList , *it, "Error" );
            tmp->setPixmap(m_tCol,SmallIcon( "messagsebox_critical" ));
            break;
        case GSInterpreterLib::Input:
            tmp=new K3ListViewItem( m_msgList , *it, "Input" );
            tmp->setPixmap(m_tCol,SmallIcon( "1leftarrow" ));
            break;
        case GSInterpreterLib::Output:
            tmp=new K3ListViewItem( m_msgList , *it, "Output" );
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
        kDebug() << "last was full line" << endl;
        if (t==m_buffer.first)
        {
            kDebug() << "appending to buffer" << endl;
            m_buffer.second +=QString::fromLocal8Bit( buf, num );
        }
        else
        {
            kDebug() << "appending from buffer" << endl;
            // sets m_lastInt to 0
            appendBuffered();
        }
    }

    if (num==128)
    {
        kDebug() << "this is full line" << endl;
        if (m_lastInt != 128)
        {
            kDebug() << "appending to buffer" << endl;
            m_buffer.first=t;
            m_buffer.second=QString::fromLocal8Bit( buf, num );
        }
        m_clearTimer.stop();
        m_clearTimer.start(20);
    }
    else
    {
        kDebug() << "this is normal line" << endl;
        if (m_lastInt == 128)
        {
            kDebug() << "appending from buffer" << endl;
            appendBuffered();
        }
        else
        {
            kDebug() << "appending directly" << endl;
            append(t,QString::fromLocal8Bit( buf, num ));
            m_clearTimer.stop();
        }
    }
    m_lastInt=num;

    //    kDebug()<< "LogWindow before split: " << msgString << " lenght" << num << endl;    

}

void GSLogWindow::clear()
{
    m_msgList ->clear();
}

#include "gvlogwindow.moc"
