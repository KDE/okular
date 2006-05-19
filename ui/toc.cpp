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
#include <qstringlist.h>
#include <klocale.h>

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
            if ( KpdfSettings::tocPageColumn() && e.hasAttribute( "Page" ) )
                setText( 1, e.attribute( "Page" ) );
        }

        TOCItem( QTreeWidgetItem *parent, TOCItem *after, const QDomElement & e )
            : QTreeWidgetItem( parent, after ), m_element( e )
        {
            setText( 0, e.tagName() );
            if ( KpdfSettings::tocPageColumn() && e.hasAttribute( "Page" ) )
                setText( 1, e.attribute( "Page" ) );
        }

        const QDomElement & element() const
        {
            return m_element;
        }

    private:
        QDomElement m_element;
};

TOC::TOC(QWidget *parent, KPDFDocument *document) : QTreeWidget(parent), m_document(document)
{
    QStringList cols;
    cols.append( i18n("Topic") );
    if (KpdfSettings::tocPageColumn())
        cols.append( i18n("Page") );
    setHeaderLabels(cols);
    setSortingEnabled(false);
    setRootIsDecorated(true);
    setAlternatingRowColors(true);
    header()->setResizeMode(QHeaderView::Stretch);
    header()->hide();
    setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(this, SIGNAL(itemClicked(QTreeWidgetItem *, int)), SLOT(slotExecuted(QTreeWidgetItem *)));
    connect(this, SIGNAL(itemActivated(QTreeWidgetItem *, int)), SLOT(slotExecuted(QTreeWidgetItem *)));
}

TOC::~TOC()
{
    m_document->removeObserver( this );
}

uint TOC::observerId() const
{
    return TOC_ID;
}

void TOC::notifySetup( const QVector< KPDFPage * > & /*pages*/, bool documentChanged )
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
            currentItem = new TOCItem( this, currentItem, e );
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
