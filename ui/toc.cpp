/***************************************************************************
 *   Copyright (C) 2004-2006 by Albert Astals Cid <tsdgeos@terra.es>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qheaderview.h>
#include <qlayout.h>
#include <qstringlist.h>
#include <qtoolbutton.h>
#include <qtreewidget.h>
#include <kicon.h>
#include <klocale.h>
#include <ktreewidgetsearchline.h>

// local includes
#include "toc.h"
#include "core/document.h"
#include "core/page.h"
#include "settings.h"
#include "core/link.h"

// uncomment following to enable a 2nd column showing the page referred
// by each tree entry note: PDF uses often references to viewports and
// they're slow when converted to page number. drop the 2nd column idea.
// to enable set TocPageColumn=true in [Nav Panel]

class TOCItem : public QTreeWidgetItem
{
    public:
        TOCItem( QTreeWidget *parent, TOCItem *after, const QDomElement & e )
            : QTreeWidgetItem( parent, after ), m_element( e )
        {
            setText( 0, e.tagName() );
            if ( Okular::Settings::tocPageColumn() && e.hasAttribute( "Page" ) )
                setText( 1, e.attribute( "Page" ) );
        }

        TOCItem( QTreeWidgetItem *parent, TOCItem *after, const QDomElement & e )
            : QTreeWidgetItem( parent, after ), m_element( e )
        {
            setText( 0, e.tagName() );
            if ( Okular::Settings::tocPageColumn() && e.hasAttribute( "Page" ) )
                setText( 1, e.attribute( "Page" ) );
        }

        const QDomElement & element() const
        {
            return m_element;
        }

        void setSelected( bool selected )
        {
            setIcon( 0, selected ? KIcon( treeWidget()->layoutDirection() == Qt::RightToLeft ? "1leftarrow" : "1rightarrow" ) : QIcon() );
        }

    private:
        QDomElement m_element;
};

TOC::TOC(QWidget *parent, Okular::Document *document) : QWidget(parent), m_document(document), m_current(0), m_currentPage(-1)
{
    QVBoxLayout *mainlay = new QVBoxLayout( this );
    mainlay->setMargin( 0 );

    QHBoxLayout *searchlay = new QHBoxLayout();
    searchlay->setMargin( 2 );
    searchlay->setSpacing( 2 );
    mainlay->addLayout( searchlay );

    m_searchLine = new KTreeWidgetSearchLine( this );
    searchlay->addWidget( m_searchLine );

    m_treeView = new QTreeWidget( this );
    mainlay->addWidget( m_treeView );
    QStringList cols;
    cols.append( i18n("Topic") );
    if (Okular::Settings::tocPageColumn())
        cols.append( i18n("Page") );
    m_treeView->setHeaderLabels( cols );
    m_treeView->setSortingEnabled( false );
    m_treeView->setRootIsDecorated( true );
    m_treeView->setAlternatingRowColors( true );
    m_treeView->header()->setResizeMode( QHeaderView::Stretch );
    m_treeView->header()->hide();
    m_treeView->setSelectionBehavior( QAbstractItemView::SelectRows );
    connect( m_treeView, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( slotExecuted( QTreeWidgetItem * ) ) );
    connect( m_treeView, SIGNAL( itemActivated( QTreeWidgetItem *, int ) ), this, SLOT( slotExecuted( QTreeWidgetItem * ) ) );
    m_searchLine->addTreeWidget( m_treeView );
}

TOC::~TOC()
{
    m_document->removeObserver( this );
}

uint TOC::observerId() const
{
    return TOC_ID;
}

void TOC::notifySetup( const QVector< Okular::Page * > & /*pages*/, bool documentChanged )
{
    if ( !documentChanged )
        return;

    // clear contents
    m_treeView->clear();
    m_searchLine->clear();
    m_current = 0;
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
    addChildren( *syn );
    emit hasTOC( true );
}

void TOC::notifyViewportChanged( bool /*smoothMove*/ )
{
    int newpage = m_document->viewport().pageNumber;
    if ( m_currentPage == newpage )
        return;

    m_currentPage = newpage;

    if ( m_current )
    {
        m_current->setSelected( false );
        m_current = 0;
    }

    QTreeWidgetItemIterator it( m_treeView );
    while ( (*it) && !m_current )
    {
        TOCItem *tmp = dynamic_cast<TOCItem*>( *it );
        int p = tmp ? getViewport( tmp->element() ).pageNumber : -1;
        if ( p == newpage )
        {
            m_current = tmp;
            if (m_current)
                m_current->setSelected( true );
        }
        ++it;
    }
}


void TOC::addChildren( const QDomNode & parentNode, QTreeWidgetItem * parentItem )
{
    // keep track of the current listViewItem
    TOCItem * currentItem = 0;
    QDomNode n = parentNode.firstChild();
    while( !n.isNull() )
    {
        // convert the node to an element (sure it is)
        QDomElement e = n.toElement();

        // insert the entry as top level (listview parented) or 2nd+ level
        if ( !parentItem )
            currentItem = new TOCItem( m_treeView, currentItem, e );
        else
            currentItem = new TOCItem( parentItem, currentItem, e );

        // descend recursively and advance to the next node
        if ( e.hasChildNodes() )
            addChildren( n, currentItem );
        n = n.nextSibling();
    }
}

void TOC::slotExecuted( QTreeWidgetItem *i )
{
    TOCItem* tocItem = dynamic_cast<TOCItem*>( i );
    // that filters clicks on [+] that for a strange reason don't seem to be TOCItem*
    if (tocItem == NULL)
        return;
    const QDomElement & e = tocItem->element();

    QString externalFileName = e.attribute( "ExternalFileName" );
    if ( !externalFileName.isEmpty() )
    {
        Okular::LinkGoto link( externalFileName, getViewport( e ) );
        m_document->processLink( &link );
    }
    else
    {
        m_document->setViewport( getViewport( e ) );
    }
}

Okular::DocumentViewport TOC::getViewport( const QDomElement &e ) const
{
    if ( e.hasAttribute( "Viewport" ) )
    {
        // if the node has a viewport, set it
        return Okular::DocumentViewport( e.attribute( "Viewport" ) );
    }
    else if ( e.hasAttribute( "ViewportName" ) )
    {
        // if the node references a viewport, get the reference and set it
        const QString & page = e.attribute( "ViewportName" );
        const QString & viewport = m_document->getMetaData( "NamedViewport", page );
        if ( !viewport.isNull() )
            return Okular::DocumentViewport( viewport );
    }
    return Okular::DocumentViewport();
}

#include "toc.moc"
