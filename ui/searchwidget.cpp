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
#include "core/document.h"
#include "conf/settings.h"

#define CLEAR_ID    1
#define LEDIT_ID    2
#define FIND_ID     3

SearchWidget::SearchWidget( QWidget * parent, KPDFDocument * document )
    : KToolBar( parent, "iSearchBar" ), m_document( document ),
    m_searchType( 0 ), m_caseSensitive( false )
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

    // 1. text line
    insertLined( QString::null, LEDIT_ID, SIGNAL( textChanged(const QString &) ),
                 this, SLOT( slotTextChanged(const QString &) ), true,
                 i18n( "Enter at least 3 letters to filter pages" ), 0/*size*/, 1 );

    // 2. clear button (uses a lineEdit slot, so it must be created after)
    insertButton( QApplication::reverseLayout() ? "clear_left" : "locationbar_erase",
                  CLEAR_ID, SIGNAL( clicked() ),
                  getLined( LEDIT_ID ), SLOT( clear() ), true,
                  i18n( "Clear filter" ), 0/*index*/ );

    // 3.1. create the popup menu for changing filtering features
    m_menu = new KPopupMenu( this );
    m_menu->insertItem( i18n("Case Sensitive"), 1 );
    m_menu->insertSeparator( 2 );
    m_menu->insertItem( i18n("Match Phrase"), 3 );
    m_menu->insertItem( i18n("Match All Words"), 4 );
    m_menu->insertItem( i18n("Match Any Word"), 5 );
    m_menu->setItemChecked( 3, true );
    connect( m_menu, SIGNAL( activated(int) ), SLOT( slotMenuChaged(int) ) );

    // 3.2. create the toolbar button that spawns the popup menu
    insertButton( "kpdf", FIND_ID, m_menu, true, i18n( "Filter Options" ), 2/*index*/ );

    // always maximize the text line
    setItemAutoSized( LEDIT_ID );
}

void SearchWidget::clearText()
{
    getLined( LEDIT_ID )->clear();
}

void SearchWidget::slotTextChanged( const QString & text )
{
    // if 0<length<3 set 'red' text and send a blank string to document
    QColor color = text.length() > 0 && text.length() < 3 ? Qt::darkRed : palette().active().text();
    KLineEdit * lineEdit = getLined( LEDIT_ID );
    lineEdit->setPaletteForegroundColor( color );
    lineEdit->setPaletteBackgroundColor( palette().active().base() );
    m_inputDelayTimer->stop();
    m_inputDelayTimer->start(333, true);
}

void SearchWidget::slotMenuChaged( int index )
{
    // update internal variables and checked state
    if ( index == 1 )
    {
        m_caseSensitive = !m_caseSensitive;
        m_menu->setItemChecked( 1, m_caseSensitive );
    }
    else if ( index >= 3 && index <= 5 )
    {
        m_searchType = index - 3;
        for ( int i = 0; i < 3; i++ )
            m_menu->setItemChecked( i + 3, m_searchType == i );
    }
    else
        return;

    // update search
    slotTextChanged( getLined( LEDIT_ID )->text() );
}

void SearchWidget::startSearch()
{
    // search text if have more than 3 chars or else clear search
    QString text = getLined( LEDIT_ID )->text();
    bool ok = true;
    if ( text.length() >= 3 )
    {
        KPDFDocument::SearchType type = !m_searchType ? KPDFDocument::AllDoc :
                                        ( (m_searchType > 1) ? KPDFDocument::GoogleAny :
                                        KPDFDocument::GoogleAll );
        ok = m_document->searchText( SW_SEARCH_ID, text, true, m_caseSensitive,
                                     type, false, qRgb( 0, 183, 255 ) );
    }
    else
        m_document->resetSearch( SW_SEARCH_ID );
    // if not found, use warning colors
    if ( !ok )
    {
        KLineEdit * lineEdit = getLined( LEDIT_ID );
        lineEdit->setPaletteForegroundColor( Qt::white );
        lineEdit->setPaletteBackgroundColor( Qt::red );
    }
}

#include "searchwidget.moc"
