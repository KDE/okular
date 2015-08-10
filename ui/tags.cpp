/***************************************************************************
 *   Copyright (C) 2015 by Saheb Preet Singh <saheb.preet@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QVBoxLayout>
#include <QTreeView>
#include <qheaderview.h>

#include "tags.h"
#include "settings.h"
#include "core/document.h"
#include "ktreeviewsearchline.h"

Tags::Tags( QWidget *parent, Okular::Document *document )
    : QWidget( parent ), m_document( document )
{
    QVBoxLayout * const mainlay = new QVBoxLayout( this );
    mainlay->setMargin( 0 );
    mainlay->setSpacing( 6 );

    m_document->addObserver( this );

    m_searchLine = new KTreeViewSearchLine( this );
    mainlay->addWidget( m_searchLine );
    m_searchLine->setCaseSensitivity( Okular::Settings::self()->tagsSearchCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive );
    m_searchLine->setRegularExpression( Okular::Settings::self()->tagsSearchRegularExpression() );
    connect( m_searchLine, SIGNAL(searchOptionsChanged()), this, SLOT(saveSearchOptions()) );

    m_treeView = new QTreeView( this );
    mainlay->addWidget( m_treeView );

    QAbstractItemModel * tagsModel = m_document->tagsModel();

    if( tagsModel )
    {
	m_treeView->setModel( tagsModel );
	m_searchLine->addTreeView( m_treeView );
	emit hasTags( true );
	//connect( tagsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), m_document, SLOT(reloadDocument()) );
    }
    else
    {
	emit hasTags( false );
    }
    m_treeView->setSortingEnabled( false );
    m_treeView->setRootIsDecorated( true );
    m_treeView->setAlternatingRowColors( true );
    m_treeView->header()->hide();
}

Tags::~Tags()
{
    m_document->removeObserver( this );
}

void Tags::notifySetup( const QVector< Okular::Page * > & /*pages*/, int /*setupFlags*/ )
{
    QAbstractItemModel * tagsModel = m_document->tagsModel();
    if( tagsModel )
    {
	m_treeView->setModel( tagsModel );
	m_searchLine->addTreeView( m_treeView );
	emit hasTags( true );
	//connect( layersModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), m_document, SLOT(reloadDocument()) );
	//connect( layersModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), m_pageView, SLOT(reloadForms()) );
    }
    else
    {
	emit hasTags( false );
    }
}

void Tags::saveSearchOptions()
{
    Okular::Settings::setTagsSearchRegularExpression( m_searchLine->regularExpression() );
    Okular::Settings::setTagsSearchCaseSensitive( m_searchLine->caseSensitivity() == Qt::CaseSensitive ? true : false );
    Okular::Settings::self()->writeConfig();
}

 #include "tags.moc"
