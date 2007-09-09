/***************************************************************************
 *   Copyright (C) 2004-2006 by Albert Astals Cid <tsdgeos@terra.es>       *
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

TOC::TOC(QWidget *parent, Okular::Document *document) : QWidget(parent), m_document(document), m_currentPage(-1)
{
    QVBoxLayout *mainlay = new QVBoxLayout( this );
    mainlay->setMargin( 0 );
    mainlay->setSpacing( 6 );

    m_searchLine = new KTreeViewSearchLine( this );
    mainlay->addWidget( m_searchLine );

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
    connect( m_treeView, SIGNAL( clicked( const QModelIndex & ) ), this, SLOT( slotExecuted( const QModelIndex & ) ) );
    connect( m_treeView, SIGNAL( activated( const QModelIndex & ) ), this, SLOT( slotExecuted( const QModelIndex & ) ) );
    m_searchLine->addTreeView( m_treeView );
}

TOC::~TOC()
{
    m_document->removeObserver( this );
}

uint TOC::observerId() const
{
    return TOC_ID;
}

void TOC::notifySetup( const QVector< Okular::Page * > & /*pages*/, int setupFlags )
{
    if ( !( setupFlags & Okular::DocumentObserver::DocumentChanged ) )
        return;

    // clear contents
    m_model->clear();
    m_currentPage = -1;

    // request synopsis description (is a dom tree)
    const Okular::DocumentSynopsis * syn = m_document->documentSynopsis();

    // if not present, disable the contents tab
    if ( !syn )
    {
        emit hasTOC( false );
        return;
    }

    // else populate the listview and enable the tab
    m_model->fill( syn );
    emit hasTOC( true );
}

void TOC::notifyViewportChanged( bool /*smoothMove*/ )
{
    int newpage = m_document->viewport().pageNumber;
    if ( m_currentPage == newpage )
        return;

    m_currentPage = newpage;

    m_model->setCurrentViewport( m_document->viewport() );
}


void TOC::reparseConfig()
{
    m_treeView->update();
}


void TOC::slotExecuted( const QModelIndex &index )
{
    if ( !index.isValid() )
        return;

    QString externalFileName = m_model->externalFileNameForIndex( index );
    Okular::DocumentViewport viewport = m_model->viewportForIndex( index );
    if ( !externalFileName.isEmpty() )
    {
        Okular::GotoAction action( externalFileName, viewport );
        m_document->processAction( &action );
    }
    else
    {
        m_document->setViewport( viewport );
    }
}

#include "toc.moc"
