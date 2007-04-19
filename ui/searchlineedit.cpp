/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "searchlineedit.h"

// qt/kde includes
#include <qapplication.h>
#include <qtimer.h>

SearchLineEdit::SearchLineEdit( QWidget * parent, Okular::Document * document )
    : KLineEdit( parent ), m_document( document ), m_minLength( 0 ),
      m_caseSensitivity( Qt::CaseInsensitive ),
      m_searchType( Okular::Document::AllDocument ), m_id( -1 ),
      m_moveViewport( false ), m_changed( false )
{
    setObjectName( "SearchLineEdit" );

    // a timer to ensure that we don't flood the document with requests to search
    m_inputDelayTimer = new QTimer(this);
    m_inputDelayTimer->setSingleShot(true);
    connect( m_inputDelayTimer, SIGNAL( timeout() ),
             this, SLOT( startSearch() ) );

    connect(this, SIGNAL( textChanged(const QString &) ), this, SLOT( slotTextChanged(const QString &) ));
}

void SearchLineEdit::clearText()
{
    clear();
}

void SearchLineEdit::setSearchCaseSensitivity( Qt::CaseSensitivity cs )
{
    m_caseSensitivity = cs;
    m_changed = true;
}

void SearchLineEdit::setSearchMinimumLength( int length )
{
    m_minLength = length;
    m_changed = true;
}

void SearchLineEdit::setSearchType( Okular::Document::SearchType type )
{
    m_searchType = type;
    m_changed = true;
}

void SearchLineEdit::setSearchId( int id )
{
    m_id = id;
    m_changed = true;
}

void SearchLineEdit::setSearchColor( const QColor &color )
{
    m_color = color;
    m_changed = true;
}

void SearchLineEdit::setSearchMoveViewport( bool move )
{
    m_moveViewport = move;
}

void SearchLineEdit::restartSearch()
{
    m_inputDelayTimer->stop();
    m_inputDelayTimer->start( 500 );
    m_changed = true;
}

void SearchLineEdit::findNext()
{
    if ( m_id == -1 || m_searchType != Okular::Document::NextMatch )
        return;

    if ( !m_changed )
        m_document->continueSearch( m_id );
    else
        startSearch();
}

void SearchLineEdit::slotTextChanged( const QString & text )
{
    QPalette qAppPalette = QApplication::palette();
    // if 0<length<minLength set 'red' text and send a blank string to document
    QColor color = text.length() < m_minLength && text.length() > 0 ? Qt::darkRed : qAppPalette.color( QPalette::Text );
    QPalette pal = palette();
    pal.setColor( QPalette::Base, qAppPalette.color( QPalette::Base ) );
    pal.setColor( QPalette::Text, color );
    setPalette( pal );
    restartSearch();
}

void SearchLineEdit::startSearch()
{
    if ( m_id == -1 || !m_color.isValid() )
        return;

    if ( m_changed && m_searchType == Okular::Document::NextMatch )
    {
        m_document->resetSearch( m_id );
    }
    m_changed = false;
    // search text if have more than 3 chars or else clear search
    QString thistext = text();
    bool ok = true;
    if ( thistext.length() >= qMax( m_minLength, 1 ) )
    {
        ok = m_document->searchText( m_id, thistext, true, m_caseSensitivity,
                                     m_searchType, m_moveViewport, m_color );
    }
    else
        m_document->resetSearch( m_id );
    // if not found, use warning colors
    if ( !ok )
    {
        QPalette pal = palette();
        pal.setColor( QPalette::Base, Qt::red );
        pal.setColor( QPalette::Text, Qt::white );
        setPalette( pal );
    }
}

#include "searchlineedit.moc"
