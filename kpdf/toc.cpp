/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qheader.h>

#include "toc.h"
#include "page.h"
#include "document.h"

class TOCItem : public KListViewItem
{
	public:
		TOCItem(KListView *parent, TOCItem *after, QString name, const QDomElement & e) :
			KListViewItem(parent, after, name), m_element(e)
		{
		}

		TOCItem(KListViewItem *parent, TOCItem *after, QString name, const QDomElement & e) :
			KListViewItem(parent, after, name), m_element(e)
		{
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
    addColumn("");
    header() -> hide();
    setSorting(-1);
    setRootIsDecorated(true);
    setResizeMode(AllColumns);
    connect(this, SIGNAL(executed(QListViewItem *)), this, SLOT(slotExecuted(QListViewItem *)));
}

uint TOC::observerId() const
{
    return TOC_ID;
}

void TOC::pageSetup( const QValueVector<KPDFPage*> & pages, bool documentChanged)
{
    if ( !documentChanged || pages.size() < 1 )
        return;

    clear();
    const DocumentSynopsis * syn = m_document->documentSynopsis();
    if ( syn )
    {
        
    }

    emit hasTOC( syn );
}

void TOC::slotExecuted(QListViewItem *i)
{
//	TOCItem *ti = dynamic_cast<TOCItem*>(i);
    //FIXME
	//KPDFLink l( ti->getAction() );
	//m_document->processLink( &l );
}

#include "toc.moc"
