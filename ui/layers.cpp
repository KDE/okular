/***************************************************************************
 *   Copyright (C) 2015 by Saheb Preet Singh <saheb.preet@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "layers.h"
#include "settings.h"

// qt/kde includes
#include <QVBoxLayout>
#include <QTreeView>
#include <qheaderview.h>

// local includes
#include "core/document.h"
#include "ktreeviewsearchline.h"
#include "layersitemdelegate.h"

Layers::Layers(QWidget *parent, Okular::Document *document) : QWidget(parent), m_document(document)
{
    QVBoxLayout * const mainlay = new QVBoxLayout( this );
    mainlay->setMargin( 0 );
    mainlay->setSpacing( 6 );

    m_document->addObserver( this );

    m_searchLine = new KTreeViewSearchLine( this );
    mainlay->addWidget( m_searchLine );
    m_searchLine->setCaseSensitivity( Okular::Settings::self()->contentsSearchCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive );
    m_searchLine->setRegularExpression( Okular::Settings::self()->contentsSearchRegularExpression() );
    connect( m_searchLine, SIGNAL(searchOptionsChanged()), this, SLOT(saveSearchOptions()) );

    m_treeView = new QTreeView( this );
    mainlay->addWidget( m_treeView );

    QAbstractItemModel * layersModel = m_document->layersModel();

    QAbstractItemDelegate * delegate = new LayersItemDelegate( this );
    m_treeView->setItemDelegate( delegate );

    if( layersModel )
    {
	m_treeView->setModel( layersModel );
	m_searchLine->addTreeView( m_treeView );
	emit hasLayers( true );
	connect( layersModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), m_document, SLOT(reloadDocument()) );
	connect( layersModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(repaintItems(QModelIndex,QModelIndex)) );
    }
    else
    {
	emit hasLayers( false );
    }
    m_treeView->setSortingEnabled( false );
    m_treeView->setRootIsDecorated( true );
    m_treeView->setAlternatingRowColors( true );
    m_treeView->header()->hide();
}

Layers::~Layers()
{
    m_document->removeObserver( this );
}

void Layers::notifySetup( const QVector< Okular::Page * > & /*pages*/, int /*setupFlags*/ )
{
    QAbstractItemModel * layersModel = m_document->layersModel();
    if( layersModel )
    {
	m_treeView->setModel( layersModel );
	m_searchLine->addTreeView( m_treeView );
	emit hasLayers( true );
	connect( layersModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), m_document, SLOT(reloadDocument()) );
	connect( layersModel, SIGNAL(dataChanged(const QModelIndex,const QModelIndex)), this, SLOT(repaintItems(const QModelIndex,const QModelIndex)) );
    }
    else
    {
	emit hasLayers( false );
    }
}

void Layers::saveSearchOptions()
{
    Okular::Settings::setContentsSearchRegularExpression( m_searchLine->regularExpression() );
    Okular::Settings::setContentsSearchCaseSensitive( m_searchLine->caseSensitivity() == Qt::CaseSensitive ? true : false );
    Okular::Settings::self()->writeConfig();
}

void Layers::repaintItems(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    if( topLeft == bottomRight )
	m_treeView->update( topLeft );
}

 #include "layers.moc"
