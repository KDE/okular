/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qheader.h>
#include <klocale.h>

// local includes
#include "toc.h"
#include "core/document.h"
#include "core/page.h"

// uncomment following to enable a 2nd column showing the page referred by
// each tree entry
//#define TOC_ENABLE_PAGE_COLUMN

class TOCItem : public KListViewItem
{
    public:
        TOCItem( KListView *parent, TOCItem *after, const QDomElement & e )
            : KListViewItem( parent, after, e.tagName() ), m_element( e )
        {
#ifdef TOC_ENABLE_PAGE_COLUMN
            if ( e.hasAttribute( "Page" ) )
                setText( 1, e.attribute( "Page" ) );
#endif
        }

        TOCItem( KListViewItem *parent, TOCItem *after, const QDomElement & e )
            : KListViewItem( parent, after, e.tagName() ), m_element( e )
        {
#ifdef TOC_ENABLE_PAGE_COLUMN
            if ( e.hasAttribute( "Page" ) )
                setText( 1, e.attribute( "Page" ) );
#endif
        }

        const QDomElement & element() const
        {
            return m_element;
        }

    private:
        QDomElement m_element;
};

TOC::TOC(QWidget *parent, KPDFDocument *document) : KListView(parent), m_document(document)
{
    addColumn( i18n("Topic") );
#ifdef TOC_ENABLE_PAGE_COLUMN
    addColumn( i18n("Page") );
#else
    header() -> hide();
#endif
    setSorting(-1);
    setRootIsDecorated(true);
    setResizeMode(AllColumns);
    connect(this, SIGNAL(executed(QListViewItem *)), this, SLOT(slotExecuted(QListViewItem *)));
}

uint TOC::observerId() const
{
    return TOC_ID;
}

void TOC::pageSetup( const QValueVector<KPDFPage*> & pages, bool documentChanged )
{
    if ( !documentChanged || pages.size() < 1 )
        return;

    // clear contents
    clear();

    // request synopsis description (is a dom tree)
    const DocumentSynopsis * syn = m_document->documentSynopsis();

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

void TOC::addChildren( const QDomNode & parentNode, KListViewItem * parentItem )
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
            currentItem = new TOCItem( this, currentItem, e );
        else
            currentItem = new TOCItem( parentItem, currentItem, e );

        // descend recursively and advance to the next node
        if ( e.hasChildNodes() )
            addChildren( n, currentItem );
        n = n.nextSibling();
    }
}

void TOC::slotExecuted( QListViewItem *i )
{
    const QDomElement & e = static_cast< TOCItem* >( i )->element();
    if ( e.hasAttribute( "PageNumber" ) )
    {
        // if the node has a page number, follow it
        m_document->setCurrentPage( e.attribute( "Page" ).toUInt() );
    }
    else if ( e.hasAttribute( "PageName" ) )
    {
        // if the node has a named reference, ask for conversion
        const QString & page = e.attribute( "PageName" );
        const QString & pageNumber = m_document->getMetaData( "NamedLink", page );
        bool ok;
        int n = pageNumber.toUInt( &ok );
        if ( ok )
            m_document->setCurrentPage( n );
    }
}

#include "toc.moc"
