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

class GList;

class PDFDoc;
class UnicodeMap;

class TOC : public KListView
{
Q_OBJECT
	public:
		TOC(QWidget *parent);

		bool generate(PDFDoc *doc);

	signals:
		void execute(LinkAction *action);

	private:
		void addKids(KListViewItem *last, GList *items, UnicodeMap *uMap);
		QString getTitle(Unicode *u, int length, UnicodeMap *uMap) const;

	private slots:
		void slotExecuted(QListViewItem *i);
};

#endif
