/***************************************************************************
 *   Copyright (C) 1997-2005 the KGhostView authors. See file GV_AUTHORS.  *
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   Many portions of this file are based on kghostview's kpswidget code   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qtextedit.h>
#include <qlabel.h>
#include <qvbox.h>

#include <kglobalsettings.h>
#include <kurllabel.h>
#include <klocale.h>

#include "gvlogwindow.h"

GVLogWindow::GVLogWindow( const QString& caption, 
                      QWidget* parent, const char* name) :
    KDialogBase( parent, name, false, caption, User1|Close, Close, false, 
                 KStdGuiItem::clear() )
{
    QVBox * display = makeVBoxMainWidget();

    m_errorIndication = new QLabel( "", display, "logview-label" );
    m_errorIndication->hide();

    m_configureGS = new KURLLabel( i18n( "Configure Ghostscript" ), QString::null, display );
    m_configureGS->hide();

    m_logView = new QTextEdit( display, "logview" );
    m_logView->setTextFormat( Qt::PlainText );
    m_logView->setReadOnly( true );
    m_logView->setWordWrap( QTextEdit::NoWrap );
    m_logView->setFont( KGlobalSettings::fixedFont() );
    m_logView->setMinimumWidth( 80 * fontMetrics().width( " " ) );

    connect( this, SIGNAL( user1Clicked() ), SLOT( clear() ) );
    connect( m_configureGS, SIGNAL( leftClickedURL() ), SLOT( emitConfigureGS() ) );
}

void GVLogWindow::emitConfigureGS() {
	emit configureGS();
}

void GVLogWindow::append( const QString& message )
{
    m_logView->append( message );
    this->show();
}

void GVLogWindow::append( char* buf, int num )
{
    m_logView->append( QString::fromLocal8Bit( buf, num ) );
//    if( _showLogWindow )
    this->show();
}

void GVLogWindow::clear()
{
    m_logView->clear();
    m_errorIndication->clear();
}

void GVLogWindow::setLabel( const QString& text, bool showConfigureGS )
{
	m_errorIndication->setText( text );
	m_errorIndication->show();
	if ( showConfigureGS ) m_configureGS->show();
	else m_configureGS->hide();
}

#include "gvlogwindow.moc"
