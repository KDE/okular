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
#include <kaction.h>
#include <kactioncollection.h>
#include <kconfigbase.h>
#include <klocale.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kpopupmenu.h>
#include <ktoolbarbutton.h>

// local includes
#include "searchwidget.h"
#include "document.h"

//#include <qiconset.h>
SearchWidget::SearchWidget( QWidget * parent, KPDFDocument * document )
    : QHBox( parent ), m_document( document ), m_caseSensitive( false )
{
    setMargin( 4 );

    // clear button
    KToolBarButton * m_clearButton = new KToolBarButton( SmallIcon("locationbar_erase"), 1, this );
    QToolTip::add( m_clearButton, i18n( "Clear filter" ) );

    // line edit
    m_lineEdit = new KLineEdit( this );
    m_lineEdit->setFrame( QFrame::Sunken );
    connect( m_lineEdit, SIGNAL(textChanged(const QString &)), SLOT(slotTextChanged(const QString &)) );
    connect( m_clearButton, SIGNAL(clicked()), m_lineEdit, SLOT(clear()) );
    QToolTip::add( m_lineEdit, i18n( "Enter at least 3 letters to filter pages" ) );

    // change case button and menu
/*    KToolBarButton * search = new KToolBarButton( SmallIcon("find"), 2, this );
    m_caseMenu = new KPopupMenu( search );
    m_caseMenu->insertItem( i18n("Case Insensitive"), 1 );
    m_caseMenu->insertItem( i18n("Case Sensitive"), 2 );
    m_caseMenu->setItemChecked( 1, true );
    connect( m_caseMenu, SIGNAL( activated(int) ), SLOT( slotChangeCase(int) ) );
    search->setPopup( m_caseMenu );
*/
    int sideLength = m_lineEdit->sizeHint().height();
    m_clearButton->setMinimumSize( QSize( sideLength, sideLength ) );
 //   search->setMinimumSize( QSize( sideLength, sideLength ) );
}

void SearchWidget::setupActions( KActionCollection * ac, KConfigGroup * config )
{
    KToggleAction * ss = new KToggleAction( i18n( "Show Search Bar" ), 0, ac, "show_searchbar" );
    ss->setCheckedState(i18n("Hide Search Bar"));
    connect( ss, SIGNAL( toggled( bool ) ), SLOT( slotToggleSearchBar( bool ) ) );

    ss->setChecked( config->readBoolEntry( "ShowSearchBar", false ) );
    slotToggleSearchBar( ss->isChecked() );
}

void SearchWidget::saveSettings( KConfigGroup * config )
{
    config->writeEntry( "ShowSearchBar", isShown() );
}

void SearchWidget::slotTextChanged( const QString & text )
{
    if ( text.length() > 2 || text.isEmpty() )
    {
        m_lineEdit->setPaletteForegroundColor( palette().active().text() );
        m_document->slotSetFilter( text, m_caseSensitive );
    }
    else
    {
        m_lineEdit->setPaletteForegroundColor( Qt::red );
        m_document->slotSetFilter( QString::null, m_caseSensitive );
    }
}

void SearchWidget::slotChangeCase( int index )
{
    bool newState = (index == 2);
    if ( newState != m_caseSensitive )
    {
        m_caseSensitive = newState;
        m_caseMenu->setItemChecked( 1, !m_caseSensitive );
        m_caseMenu->setItemChecked( 2, m_caseSensitive );
        slotTextChanged( m_lineEdit->text() );
    }
}

void SearchWidget::slotToggleSearchBar( bool visible )
{
    setShown( visible );
    if ( !visible )
        m_document->slotSetFilter( QString::null, m_caseSensitive );
}

#include "searchwidget.moc"
