/***************************************************************************
 *   Copyright (C) 2004-2006 by Albert Astals Cid <aacid@kde.org>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "toc.h"

// qt/kde includes
#include <qdom.h>
#include <qheaderview.h>
#include <qlayout.h>
#include <qtreeview.h>

#include <klineedit.h>

// local includes
#include "ktreeviewsearchline.h"
#include "pageitemdelegate.h"
#include "tocmodel.h"
#include "core/action.h"
#include "core/document.h"
#include "settings.h"

TOC::TOC(QWidget *parent, Okular::Document *document) : QWidget(parent), m_document(document)
{
    QVBoxLayout *mainlay = new QVBoxLayout( this );
    mainlay->setMargin( 0 );
    mainlay->setSpacing( 6 );

    m_searchLine = new KTreeViewSearchLine( this );
    mainlay->addWidget( m_searchLine );
    m_searchLine->setCaseSensitivity( Okular::Settings::self()->contentsSearchCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive );
    m_searchLine->setRegularExpression( Okular::Settings::self()->contentsSearchRegularExpression() );
    connect( m_searchLine, SIGNAL(searchOptionsChanged()), this, SLOT(saveSearchOptions()) );

    m_treeView = new QTreeView( this );
    mainlay->addWidget( m_treeView );
    m_model = new TOCModel( document, m_treeView );
    m_treeView->setModel( m_model );
    m_treeView->setSortingEnabled( false );
    m_treeView->setRootIsDecorated( true );
    m_treeView->setAlternatingRowColors( true );
    m_treeView->setItemDelegate( new PageItemDelegate( m_treeView ) );
    m_treeView->header()->hide();
    m_treeView->setSelectionBehavior( QAbstractItemView::SelectRows );
    connect( m_treeView, SIGNAL(clicked(QModelIndex)), this, SLOT(slotExecuted(QModelIndex)) );
    connect( m_treeView, SIGNAL(activated(QModelIndex)), this, SLOT(slotExecuted(QModelIndex)) );
    m_searchLine->addTreeView( m_treeView );
}

TOC::~TOC()
{
    m_document->removeObserver( this );
}

void TOC::notifySetup( const QVector< Okular::Page * > & /*pages*/, int setupFlags )
{
    if ( !( setupFlags & Okular::DocumentObserver::DocumentChanged ) )
        return;

    // clear contents
    m_model->clear();

    // request synopsis description (is a dom tree)
    const Okular::DocumentSynopsis * syn = m_document->documentSynopsis();
    if ( !syn )
    {
        if ( m_document->isOpened() )
        {
            // Make sure we clear the reload old model data
            m_model->setOldModelData( 0, QVector<QModelIndex>() );
        }
        emit hasTOC( false );
        return;
    }

    m_model->fill( syn );
    emit hasTOC( !m_model->isEmpty() );
}

void TOC::notifyCurrentPageChanged( int, int )
{
    m_model->setCurrentViewport( m_document->viewport() );
}

void TOC::prepareForReload()
{
    if( m_model->isEmpty() )
        return;

    const QVector<QModelIndex> list = expandedNodes();
    TOCModel *m = m_model;
    m_model = new TOCModel( m_document, m_treeView );
    m_model->setOldModelData( m, list );
    m->setParent( 0 );
}

void TOC::rollbackReload()
{
    if( !m_model->hasOldModelData() )
        return;

    TOCModel *m = m_model;
    m_model = m->clearOldModelData();
    m_model->setParent( m_treeView );
    delete m;
}

void TOC::finishReload()
{
    m_treeView->setModel( m_model );
    m_model->setParent( m_treeView );
}

QVector<QModelIndex> TOC::expandedNodes( const QModelIndex &parent ) const
{
    QVector<QModelIndex> list;
    for ( int i = 0; i < m_model->rowCount( parent ); i++ )
    {
        const QModelIndex index = m_model->index( i, 0, parent );
        if ( m_treeView->isExpanded( index ) )
        {
            list << index;
        }
        if ( m_model->hasChildren( index ) )
        {
            list << expandedNodes( index );
        }
    }
    return list;
}

void TOC::reparseConfig()
{
    m_searchLine->setCaseSensitivity( Okular::Settings::contentsSearchCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive );
    m_searchLine->setRegularExpression( Okular::Settings::contentsSearchRegularExpression() );
    m_treeView->update();
}


void TOC::slotExecuted( const QModelIndex &index )
{
    if ( !index.isValid() )
        return;

    QString url = m_model->urlForIndex( index );
    if ( !url.isEmpty() )
    {
        Okular::BrowseAction action( url );
        m_document->processAction( &action );
        return;
    }

    QString externalFileName = m_model->externalFileNameForIndex( index );
    Okular::DocumentViewport viewport = m_model->viewportForIndex( index );
    if ( !externalFileName.isEmpty() )
    {
        Okular::GotoAction action( externalFileName, viewport );
        m_document->processAction( &action );
    }
    else if ( viewport.isValid() )
    {
        m_document->setViewport( viewport );
    }
}

void TOC::saveSearchOptions()
{
    Okular::Settings::setContentsSearchRegularExpression( m_searchLine->regularExpression() );
    Okular::Settings::setContentsSearchCaseSensitive( m_searchLine->caseSensitivity() == Qt::CaseSensitive ? true : false );
    Okular::Settings::self()->writeConfig();
}

#include "toc.moc"
