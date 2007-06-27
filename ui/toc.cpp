/***************************************************************************
 *   Copyright (C) 2004-2006 by Albert Astals Cid <tsdgeos@terra.es>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qheader.h>
#include <qvariant.h>
#include <klocale.h>

// local includes
#include "toc.h"
#include "core/link.h"
#include "core/page.h"

// uncomment following to enable a 2nd column showing the page referred
// by each tree entry note: PDF uses often references to viewports and
// they're slow when converted to page number. drop the 2nd column idea.
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
            setMultiLinesEnabled(true);
        }

        TOCItem( KListViewItem *parent, TOCItem *after, const QDomElement & e )
            : KListViewItem( parent, after, e.tagName() ), m_element( e )
        {
#ifdef TOC_ENABLE_PAGE_COLUMN
            if ( e.hasAttribute( "Page" ) )
                setText( 1, e.attribute( "Page" ) );
#endif
            setMultiLinesEnabled(true);
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
    // the next line causes bug:147233
//    setResizeMode(AllColumns);
    setAllColumnsShowFocus(true);
    connect(this, SIGNAL(clicked(QListViewItem *)), this, SLOT(slotExecuted(QListViewItem *)));
    connect(this, SIGNAL(returnPressed(QListViewItem *)), this, SLOT(slotExecuted(QListViewItem *)));
}

TOC::~TOC()
{
    m_document->removeObserver( this );
}

uint TOC::observerId() const
{
    return TOC_ID;
}

void TOC::notifySetup( const QValueVector< KPDFPage * > & /*pages*/, bool documentChanged )
{
    if ( !documentChanged )
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

        // open/keep close the item
        bool isOpen = false;
        if ( e.hasAttribute( "Open" ) )
            isOpen = QVariant( e.attribute( "Open" ) ).toBool();
        currentItem->setOpen( isOpen );

        n = n.nextSibling();
    }
}

void TOC::slotExecuted( QListViewItem *i )
{
    TOCItem* tocItem = dynamic_cast<TOCItem*>( i );
    // that filters clicks on [+] that for a strange reason don't seem to be TOCItem*
    if (tocItem == NULL)
        return;
    const QDomElement & e = tocItem->element();

    QString externalFileName = e.attribute( "ExternalFileName" );
    if ( !externalFileName.isEmpty() )
    {
        KPDFLinkGoto link( externalFileName, getViewport( e ) );
        m_document->processLink( &link );
    }
    else
    {
        m_document->setViewport( getViewport( e ), TOC_ID );
    }
}

DocumentViewport TOC::getViewport( const QDomElement &e ) const
{
    if ( e.hasAttribute( "Viewport" ) )
    {
        // if the node has a viewport, set it
        return DocumentViewport( e.attribute( "Viewport" ) );
    }
    else if ( e.hasAttribute( "ViewportName" ) )
    {
        // if the node references a viewport, get the reference and set it
        const QString & page = e.attribute( "ViewportName" );
        const QString & viewport = m_document->getMetaData( "NamedViewport", page );
        if ( !viewport.isNull() )
            return DocumentViewport( viewport );
    }
    return DocumentViewport();
}

#include "toc.moc"
