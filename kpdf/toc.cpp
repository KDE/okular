/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qheader.h>

#include "goo/GList.h"

#include "xpdf/GlobalParams.h"
#include "xpdf/PDFDoc.h"
#include "xpdf/Outline.h"
#include "xpdf/UnicodeMap.h"

#include "toc.h"

class TOCItem : public KListViewItem
{
	public:
		TOCItem(KListView *parent, TOCItem *after, QString name, LinkAction *action) :
			KListViewItem(parent, after, name), m_action(action)
		{
		}

		TOCItem(KListViewItem *parent, TOCItem *after, QString name, LinkAction *action) :
			KListViewItem(parent, after, name), m_action(action)
		{
		}

		LinkAction *getAction() const
		{
			return m_action;
		}

	private:
		LinkAction *m_action;
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

uint TOC::observerId()
{
	return TOC_ID;
}

void TOC::pageSetup( const QValueVector<KPDFPage*> & /*pages*/, bool documentChanged)
{
	if (documentChanged)
	{
		GList *items, *kids;
		OutlineItem *item;
		UnicodeMap *uMap;
		GString *enc;
		TOCItem *last;

		clear();
		last = 0;
		Outline *out = m_document->outline();
		if (out) items = out->getItems();
		else items = 0;
		if (items && items->getLength() > 0)
		{
			enc = new GString("Latin1");
			uMap = globalParams->getUnicodeMap(enc);
			delete enc;

			for (int i = 0; i < items->getLength(); ++i)
			{
				item = (OutlineItem *)items->get(i);

				last = new TOCItem(this, last, getTitle(item->getTitle(), item->getTitleLength(), uMap), item->getAction());
				item->open();
				if ((kids = item->getKids()))
				{
					addKids(last, kids, uMap);
				}
			}
// 			inform we have TOC
		}
// 			inform we DO NOT have TOC
	}
}


void TOC::addKids(KListViewItem *parent, GList *items, UnicodeMap *uMap)
{
	GList *kids;
	OutlineItem *item;
	TOCItem *last;

	last = 0;
	if (items && items->getLength() > 0)
	{
		for (int i = 0; i < items->getLength(); ++i)
		{
			item = (OutlineItem *)items->get(i);

			last = new TOCItem(parent, last, getTitle(item->getTitle(), item->getTitleLength(), uMap), item->getAction());
			item->open();
			if ((kids = item->getKids()))
			{
				addKids(last, kids, uMap);
			}
		}
	}
}

QString TOC::getTitle(Unicode *u, int length, UnicodeMap *uMap) const
{
	GString *title;
	QString s;
	int n;
	char buf[8];

	title = new GString();
	for (int j = 0; j < length; ++j)
	{
		n = uMap->mapUnicode(u[j], buf, sizeof(buf));
		title->append(buf, n);
	}
	s = title->getCString();
	delete title;
	return s;
}

void TOC::slotExecuted(QListViewItem *i)
{
	TOCItem *ti = dynamic_cast<TOCItem*>(i);
	emit execute(ti -> getAction());
}

#include "toc.moc"
