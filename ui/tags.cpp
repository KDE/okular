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
#include "okular/core/area.h"
#include "core/generator.h"

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
	connect( tagsModel, SIGNAL(activated(QModelIndex)), this, SLOT(highlightDocument(QModelIndex)) );
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
	connect( m_treeView, SIGNAL(activated(QModelIndex)), this, SLOT(highlightDocument(QModelIndex)) );
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

void Tags::highlightDocument( QModelIndex index )
{
    QMap<QString, QVariant> boundingRects = index.data( Okular::Generator::TagsItemBoundingRects ).toMap();
    QColor color( 255, 227, 0 );
    QMap<QString, QVariant>::iterator it, endIt;
    QList<QVariant>::iterator lIt, lEndIt;
    if( ! boundingRects.isEmpty() )
    {
	endIt = boundingRects.end();
	it = boundingRects.begin();
	int firstPage = it.key().toInt();
	double x = -1, y = -1;
	for( ; it != endIt; it ++ )
	{
	    QList<QVariant> rectsList = it.value().toList();
	    if( ! rectsList.isEmpty() )
	    {
		Okular::RegularAreaRect * rect = new Okular::RegularAreaRect();
		lEndIt = rectsList.end();
		for( lIt = rectsList.begin(); lIt != lEndIt; lIt ++ )
		{
		    rect->appendShape( Okular::NormalizedRect::fromQRectF( ( * lIt ).toRectF() ) );
		    if( x == -1 && y == -1 )
		    {
			x = ( *lIt ).toRectF().x();
			y = ( *lIt ).toRectF().y();
		    }
		}
		m_document->setPageTextSelection( it.key().toInt(), rect, color );
	    }
	}
	Okular::DocumentViewport viewport( firstPage );
	viewport.rePos.enabled = true;
        viewport.rePos.normalizedX = x;
	viewport.rePos.normalizedY = y;
        m_document->setViewport( viewport, 0, true );
    }
}

#include "tags.moc"