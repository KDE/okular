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
#include <qaction.h>
#include <qapplication.h>
#include <qsizepolicy.h>
#include <qtimer.h>
#include <qtoolbutton.h>
#include <kicon.h>
#include <klocale.h>
#include <kiconloader.h>
#include <klineedit.h>

// local includes
#include "searchwidget.h"
#include "core/document.h"
#include "settings.h"

SearchWidget::SearchWidget( QWidget * parent, KPDFDocument * document )
    : QToolBar( parent ), m_document( document ),
    m_searchType( 0 )
{
    setObjectName( "iSearchBar" );
    // change toolbar appearance
    setIconSize(QSize(16, 16));
    setMovable( false );
    QSizePolicy sp = sizePolicy();
    sp.setVerticalPolicy( QSizePolicy::Minimum );
    setSizePolicy( sp );

    // a timer to ensure that we don't flood the document with requests to search
    m_inputDelayTimer = new QTimer(this);
    m_inputDelayTimer->setSingleShot(true);
    connect( m_inputDelayTimer, SIGNAL( timeout() ),
             this, SLOT( startSearch() ) );

    // 2. text line
    m_lineEdit = new KLineEdit(this);
    m_lineEdit->setToolTip(i18n( "Enter at least 3 letters to filter pages" ));
    connect(m_lineEdit, SIGNAL( textChanged(const QString &) ), this, SLOT( slotTextChanged(const QString &) ));
    addWidget(m_lineEdit);

    // 3.1. create the popup menu for changing filtering features
    m_menu = new QMenu( this );
    m_caseSensitiveAction = m_menu->addAction( i18n("Case Sensitive") );
    m_menu->addSeparator();
    m_matchPhraseAction = m_menu->addAction( i18n("Match Phrase") );
    m_marchAllWordsAction = m_menu->addAction( i18n("Match All Words") );
    m_marchAnyWordsAction = m_menu->addAction( i18n("Match Any Word") );

    m_caseSensitiveAction->setCheckable( true );
    QActionGroup *actgrp = new QActionGroup( this );
    m_matchPhraseAction->setCheckable( true );
    m_matchPhraseAction->setActionGroup( actgrp );
    m_marchAllWordsAction->setCheckable( true );
    m_marchAllWordsAction->setActionGroup( actgrp );
    m_marchAnyWordsAction->setCheckable( true );
    m_marchAnyWordsAction->setActionGroup( actgrp );

    m_marchAllWordsAction->setChecked( true );
    connect( m_menu, SIGNAL( triggered(QAction *) ), SLOT( slotMenuChaged(QAction*) ) );

    // 3.2. create the toolbar button that spawns the popup menu
    QToolButton *optionsMenuAction =  new QToolButton( this );
    addWidget(optionsMenuAction);
    optionsMenuAction->setIcon( KIcon( "okular" ) );
    optionsMenuAction->setToolTip( i18n( "Filter Options" ) );
    optionsMenuAction->setPopupMode( QToolButton::InstantPopup );
    optionsMenuAction->setMenu( m_menu );
}

void SearchWidget::clearText()
{
    m_lineEdit->clear();
}

void SearchWidget::slotTextChanged( const QString & text )
{
    QPalette qAppPalette = QApplication::palette();
    // if 0<length<3 set 'red' text and send a blank string to document
    QColor color = text.length() < 3 && text.length() > 0 ? Qt::darkRed : qAppPalette.color( QPalette::Text );
    QPalette pal = m_lineEdit->palette();
    pal.setColor( QPalette::Base, qAppPalette.color( QPalette::Base ) );
    pal.setColor( QPalette::Text, color );
    m_lineEdit->setPalette( pal );
    m_inputDelayTimer->stop();
    m_inputDelayTimer->start(333);
}

void SearchWidget::slotMenuChaged( QAction * act )
{
    // update internal variables and checked state
    if ( act == m_caseSensitiveAction )
    {
        // do nothing, just update the search
    }
    else if ( act == m_matchPhraseAction )
    {
        m_searchType = 0;
    }
    else if ( act == m_marchAllWordsAction )
    {
        m_searchType = 1;
    }
    else if ( act == m_marchAnyWordsAction )
    {
        m_searchType = 2;
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
        bool caseSensitive = m_caseSensitiveAction->isChecked();
        KPDFDocument::SearchType type = !m_searchType ? KPDFDocument::AllDoc :
                                        ( (m_searchType > 1) ? KPDFDocument::GoogleAny :
                                        KPDFDocument::GoogleAll );
        ok = m_document->searchText( SW_SEARCH_ID, text, true, caseSensitive,
                                     type, false, qRgb( 0, 183, 255 ) );
    }
    else
        m_document->resetSearch( SW_SEARCH_ID );
    // if not found, use warning colors
    if ( !ok )
    {
        QPalette pal = m_lineEdit->palette();
        pal.setColor( QPalette::Base, Qt::red );
        pal.setColor( QPalette::Text, Qt::white );
        m_lineEdit->setPalette( pal );
    }
}

#include "searchwidget.moc"
