/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// TODO
// #include <qheaderview.h>
// #include <qlabel.h>
// #include <qlayout.h>
// #include <qlist.h>
// #include <qstringlist.h>
// #include <qstring.h>
// #include <qtreewidget.h>
// 
// #include <kdebug.h>
// #include <kicon.h>
// #include <klocale.h>
// #include <ktreewidgetsearchline.h>
// 
// #include "gvlogwindow.h"
// 
// GSLogWindow::GSLogWindow( QWidget* parent )
//  : KVBox( parent )
// {
//     kDebug() << "Starting logwindow";
// 
//     layout()->setSpacing( 2 );
//     QWidget *searchWidget = new QWidget( this );
//     QSizePolicy sp = searchWidget->sizePolicy();
//     sp.setVerticalPolicy( QSizePolicy::Minimum );
//     searchWidget->setSizePolicy( sp );
//     QHBoxLayout *searchlay = new QHBoxLayout( searchWidget );
//     searchlay->setSpacing( 2 );
//     searchlay->setMargin( 2 );
// 
//     m_searchLine = new KTreeWidgetSearchLine();
//     searchlay->addWidget( m_searchLine );
// 
//     m_msgList = new QTreeWidget( this );
//     QStringList cols;
//     cols.append( i18n("Messages") );
//     m_msgList->setHeaderLabels( cols );
//     m_msgList->setSortingEnabled( false );
//     m_msgList->setRootIsDecorated( false );
//     m_msgList->setAlternatingRowColors( true );
//     m_msgList->header()->resizeSection( 1, 0 );
//     m_msgList->setSelectionBehavior( QAbstractItemView::SelectRows );
//     m_searchLine->addTreeWidget( m_msgList );
// 
//     QList< int > searchCols;
//     searchCols.append(0);
//     m_searchLine -> setSearchColumns (searchCols);
// 
//     m_clearTimer.setSingleShot( false );
//     connect( &m_clearTimer, SIGNAL(timeout()), this, SLOT(appendBuffered()));
// }
// 
// bool GSLogWindow::event( QEvent * event )
// {
// // FIXME: is this stuff needed?
// /*
//     KVBox::event(event);
//     if ( event->type() == QEvent::Reparent && ( m_msgList->childCount() ) )
//     {
//         int w=( m_msgList->firstChild() ) -> width(  m_msgList->fontMetrics() , m_msgList, m_tCol);
//         kDebug() << "new width = " << w;
//         m_msgList->setColumnWidth(m_tCol, w);
//     }
//     return true;
// */
//     return KVBox::event(event);
// }
// 
// void GSLogWindow::append( GSInterpreterLib::MessageType t, const QString &text)
// {
//     //kDebug() << "Appending: " << text;
//     kDebug() << "last int: " << m_lastInt;
//     QStringList l=text.trimmed().split("\n",QString::SkipEmptyParts);
//     QStringList::Iterator it=l.begin(), end=l.end();
//     while (it!=end)
//     {
// 
//     QTreeWidgetItem* tmp = 0;
//     switch(t)
//     {
//         case GSInterpreterLib::Error:
//             tmp = new QTreeWidgetItem( m_msgList );
//             tmp->setText( 0, *it );
//             tmp->setIcon( 0, KIcon( "dialog-error" ) );
//             break;
//         case GSInterpreterLib::Input:
//             tmp = new QTreeWidgetItem( m_msgList );
//             tmp->setText( 0, *it );
//             tmp->setIcon( 0, KIcon( "arrow-left" ) );
//             break;
//         case GSInterpreterLib::Output:
//             tmp = new QTreeWidgetItem( m_msgList );
//             tmp->setText( 0, *it );
//             tmp->setIcon( 0, KIcon( "arrow-right" ) );
//             break;
//     }
//     ++it;
//     }
// }
// 
// void GSLogWindow::append( GSInterpreterLib::MessageType t, const char* buf, int num )
// {
//     // ghostscript splits messages longer then 128 to chunks, handle this properly
//     if (m_lastInt == 128)
//     {
//         kDebug() << "last was full line";
//         if (t==m_buffer.first)
//         {
//             kDebug() << "appending to buffer";
//             m_buffer.second +=QString::fromLocal8Bit( buf, num );
//         }
//         else
//         {
//             kDebug() << "appending from buffer";
//             // sets m_lastInt to 0
//             appendBuffered();
//         }
//     }
// 
//     if (num==128)
//     {
//         kDebug() << "this is full line";
//         if (m_lastInt != 128)
//         {
//             kDebug() << "appending to buffer";
//             m_buffer.first=t;
//             m_buffer.second=QString::fromLocal8Bit( buf, num );
//         }
//         m_clearTimer.stop();
//         m_clearTimer.start(20);
//     }
//     else
//     {
//         kDebug() << "this is normal line";
//         if (m_lastInt == 128)
//         {
//             kDebug() << "appending from buffer";
//             appendBuffered();
//         }
//         else
//         {
//             kDebug() << "appending directly";
//             append(t,QString::fromLocal8Bit( buf, num ));
//             m_clearTimer.stop();
//         }
//     }
//     m_lastInt=num;
// 
//     //    kDebug()<< "LogWindow before split: " << msgString << " length: " << num;
// 
// }
// 
// void GSLogWindow::clear()
// {
//     m_msgList ->clear();
// }
// 
// #include "gvlogwindow.moc"
