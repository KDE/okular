/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qlayout.h>
#include <qmenu.h>
#include <qtoolbutton.h>
#include <kicon.h>
#include <klocale.h>
#include <kpushbutton.h>

// local includes
#include "findbar.h"
#include "searchlineedit.h"
#include "core/document.h"

FindBar::FindBar( Okular::Document * document, QWidget * parent )
  : QWidget( parent )
{
    QHBoxLayout * lay = new QHBoxLayout( this );
    lay->setMargin( 2 );

    QToolButton * closeBtn = new QToolButton( this );
    closeBtn->setIcon( KIcon( "fileclose" ) );
    closeBtn->setIconSize( QSize( 24, 24 ) );
    closeBtn->setToolTip( i18n( "Close" ) );
    closeBtn->setAutoRaise( true );
    lay->addWidget( closeBtn );

    m_text = new SearchLineEdit( this, document );
    m_text->setSearchCaseSensitivity( Qt::CaseInsensitive );
    m_text->setSearchMinimumLength( 0 );
    m_text->setSearchType( Okular::Document::NextMatch );
    m_text->setSearchId( PART_SEARCH_ID );
    m_text->setSearchColor( qRgb( 255, 255, 64 ) );
    m_text->setSearchMoveViewport( true );
    lay->addWidget( m_text );

    KPushButton * findNextBtn = new KPushButton( KIcon( "find-next" ), i18n( "Find Next" ), this );
    lay->addWidget( findNextBtn );

    QPushButton * optionsBtn = new QPushButton( this );
    optionsBtn->setText( i18n( "Options" ) );
    QMenu * optionsMenu = new QMenu();
    m_caseSensitiveAct = optionsMenu->addAction( i18n( "Case sensitive" ) );
    m_caseSensitiveAct->setCheckable( true );
    optionsBtn->setMenu( optionsMenu );
    lay->addWidget( optionsBtn );

    connect( closeBtn, SIGNAL( clicked() ), this, SLOT( close() ) );
    connect( findNextBtn, SIGNAL( clicked() ), this, SLOT( findNext() ) );
    connect( m_caseSensitiveAct, SIGNAL( toggled( bool ) ), this, SLOT( caseSensitivityChanged() ) );

    hide();
}

FindBar::~FindBar()
{
}

QString FindBar::text() const
{
    return m_text->text();
}

Qt::CaseSensitivity FindBar::caseSensitivity() const
{
    return m_caseSensitiveAct->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
}

void FindBar::focusAndSetCursor()
{
    setFocus();
    m_text->setFocus();
}

void FindBar::findNext()
{
    m_text->findNext();
}

void FindBar::caseSensitivityChanged()
{
    m_text->setSearchCaseSensitivity( m_caseSensitiveAct->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive );
    m_text->restartSearch();
}

#include "findbar.moc"
