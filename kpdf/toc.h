/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef TOC_H
#define TOC_H

#include <klistview.h>

#include "document.h"

class GList;

class PDFDoc;
class UnicodeMap;

class TOC : public KListView, public KPDFDocumentObserver
{
Q_OBJECT
	public:
		TOC(QWidget *parent, KPDFDocument *document);

		uint observerId();
		void pageSetup( const QValueVector<KPDFPage*> & /*pages*/, bool documentChanged);

	signals:
		void execute(LinkAction *action);
		void hasTOC(bool has);

	private slots:
		void slotExecuted(QListViewItem *i);

	private:
		void addKids(KListViewItem *last, GList *items, UnicodeMap *uMap);
		QString getTitle(Unicode *u, int length, UnicodeMap *uMap) const;

		KPDFDocument *m_document;
};

#endif
