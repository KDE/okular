/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qtooltip.h>
#include <qapplication.h>
#include <qtimer.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kpopupmenu.h>
#include <ktoolbarbutton.h>

// local includes
#include "searchwidget.h"
#include "document.h"
#include "settings.h"

// uncomment following to enable the case switching button
//#define SW_ENABLE_CASE_BUTTON
#define CLEAR_ID    1
#define LEDIT_ID    2
#define FIND_ID     3

SearchWidget::SearchWidget( QWidget * parent, KPDFDocument * document )
    : KToolBar( parent, "iSearchBar" ), m_document( document ), m_caseSensitive( false )
{
    // change toolbar appearance
    setMargin( 3 );
    setFlat( true );
    setIconSize( 16 );
    setMovingEnabled( false );

    // a timer to ensure that we don't flood the document with requests to search
    m_inputDelayTimer = new QTimer(this);
    connect( m_inputDelayTimer, SIGNAL( timeout() ),
             this, SLOT( startSearch() ) );

    // line edit
    m_linedId = insertLined( QString::null, LEDIT_ID, SIGNAL( textChanged(const QString &) ),
                             this, SLOT( slotTextChanged(const QString &) ), true,
                             i18n( "Enter at least 3 letters to filter pages" ), 0/*size*/, 1 );

    // clear button (uses a lineEdit slot, so it must be created after)
    insertButton( "editclear", CLEAR_ID, SIGNAL( clicked() ),
        getLined( LEDIT_ID ), SLOT( clear() ), true,
        i18n( "Clear filter" ), 0/*index*/ );

#ifdef SW_ENABLE_CASE_BUTTON
    // create popup menu for change case button
    m_caseMenu = new KPopupMenu( this );
    m_caseMenu->insertItem( i18n("Case Insensitive"), 1 );
    m_caseMenu->insertItem( i18n("Case Sensitive"), 2 );
    m_caseMenu->setItemChecked( 1, true );
    connect( m_caseMenu, SIGNAL( activated(int) ), SLOT( slotCaseChanged(int) ) );

    // create the change case button
    insertButton( "find", FIND_ID, m_caseMenu, true,
        i18n( "Change Case" ), 2/*index*/ );
#endif

    // setStretchableWidget( lineEditWidget );
    setItemAutoSized( LEDIT_ID );
}

void SearchWidget::slotTextChanged( const QString & text )
{
    // if length<3 set 'red' text and send a blank string to document
    QColor color = text.length() < 3 ? Qt::red : palette().active().text();
    getLined( LEDIT_ID )->setPaletteForegroundColor( color );
    m_inputDelayTimer->stop();
    m_inputDelayTimer->start(333, true);
}

void SearchWidget::startSearch()
{
    QString text = getLinedText(m_linedId);
    m_document->findTextAll( text.length() < 3 ? QString::null : text, m_caseSensitive );
}

void SearchWidget::slotCaseChanged( int index )
{
    bool newState = (index == 2);
    if ( newState != m_caseSensitive )
    {
        m_caseSensitive = newState;
        m_caseMenu->setItemChecked( 1, !m_caseSensitive );
        m_caseMenu->setItemChecked( 2, m_caseSensitive );
        slotTextChanged( getLined( LEDIT_ID )->text() );
    }
}

#include "searchwidget.moc"
