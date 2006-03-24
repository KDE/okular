/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qmenu.h>
#include <qtooltip.h>
#include <qaction.h>
#include <qapplication.h>
#include <qtimer.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kiconloader.h>
#include <klineedit.h>

// local includes
#include "searchwidget.h"
#include "core/document.h"
#include "conf/settings.h"

SearchWidget::SearchWidget( QWidget * parent, KPDFDocument * document )
    : KToolBar( parent, "iSearchBar" ), m_document( document ),
    m_searchType( 0 ), m_caseSensitive( false )
{
    // change toolbar appearance
    setIconDimensions( 16 );
    setMovable( false );

    // a timer to ensure that we don't flood the document with requests to search
    m_inputDelayTimer = new QTimer(this);
    connect( m_inputDelayTimer, SIGNAL( timeout() ),
             this, SLOT( startSearch() ) );

    // 1. text line
    m_lineEdit = new KLineEdit(this);
    m_lineEdit->setToolTip(i18n( "Enter at least 3 letters to filter pages" ));
    connect(m_lineEdit, SIGNAL( textChanged(const QString &) ), this, SLOT( slotTextChanged(const QString &) ));
    addWidget(m_lineEdit);

    // 2. clear button (uses a lineEdit slot, so it must be created after)
    QAction *clearAction = addAction( KIcon(QApplication::reverseLayout() ? "clear_left" : "locationbar_erase"),
                  QString::null, m_lineEdit, SLOT( clear() ));
    clearAction->setToolTip(i18n( "Clear filter" ));

    // 3.1. create the popup menu for changing filtering features
    m_menu = new QMenu( this );
    m_caseSensitiveAction = m_menu->addAction( i18n("Case Sensitive") );
    m_menu->insertSeparator( );
    m_matchPhraseAction = m_menu->addAction( i18n("Match Phrase") );
    m_marchAllWordsAction = m_menu->addAction( i18n("Match All Words") );
    m_marchAnyWordsAction = m_menu->addAction( i18n("Match Any Word") );

    m_caseSensitiveAction->setCheckable( true );
    m_matchPhraseAction->setCheckable( true );
    m_marchAllWordsAction->setCheckable( true );
    m_marchAnyWordsAction->setCheckable( true );

    m_marchAllWordsAction->setChecked( true );
    connect( m_menu, SIGNAL( triggered(QAction *) ), SLOT( slotMenuChaged(QAction*) ) );

    // 3.2. create the toolbar button that spawns the popup menu
#warning still have to connect that to the menu
    //insertButton( "kpdf", FIND_ID, m_menu, true, i18n( "Filter Options" ), 2/*index*/ );

    // always maximize the text line
#warning port setItemAutoSized
    //setItemAutoSized( LEDIT_ID );
}

void SearchWidget::clearText()
{
    m_lineEdit->clear();
}

void SearchWidget::slotTextChanged( const QString & text )
{
    // if length<3 set 'red' text and send a blank string to document
    QColor color = text.length() < 3 ? Qt::darkRed : palette().active().text();
    KLineEdit * lineEdit = m_lineEdit;
    lineEdit->setPaletteForegroundColor( color );
    lineEdit->setPaletteBackgroundColor( palette().active().base() );
    m_inputDelayTimer->stop();
    m_inputDelayTimer->start(333, true);
}

void SearchWidget::slotMenuChaged( QAction * act )
{
    // update internal variables and checked state
    if ( act == m_caseSensitiveAction )
    {
        m_caseSensitive = !m_caseSensitive;
        m_caseSensitiveAction->setChecked( m_caseSensitive );
    }
    else if ( act == m_matchPhraseAction )
    {
        m_searchType = 0;
        m_matchPhraseAction->setChecked( true );
        m_marchAllWordsAction->setChecked( false );
        m_marchAnyWordsAction->setChecked( false );
    }
    else if ( act == m_marchAllWordsAction )
    {
        m_searchType = 1;
        m_matchPhraseAction->setChecked( false );
        m_marchAllWordsAction->setChecked( true );
        m_marchAnyWordsAction->setChecked( false );
    }
    else if ( act == m_marchAnyWordsAction )
    {
        m_searchType = 2;
        m_matchPhraseAction->setChecked( false );
        m_marchAllWordsAction->setChecked( false );
        m_marchAnyWordsAction->setChecked( true );
    }
    else
        return;

    // update search
    slotTextChanged( m_lineEdit->text() );
}

void SearchWidget::startSearch()
{
    // search text if have more than 3 chars or else clear search
    QString text = m_lineEdit->text();
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
        KLineEdit * lineEdit = m_lineEdit;
        lineEdit->setPaletteForegroundColor( Qt::white );
        lineEdit->setPaletteBackgroundColor( Qt::red );
    }
}

#include "searchwidget.moc"
