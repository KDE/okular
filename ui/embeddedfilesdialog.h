/***************************************************************************
 *   Copyright (C) 2006 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _EMBEDDEDFILESDIALOG_H_
#define _EMBEDDEDFILESDIALOG_H_

#include <kdialog.h>

class QTreeWidgetItem;
class KPDFDocument;

class EmbeddedFilesDialog : public KDialog
{
Q_OBJECT
	public:
		EmbeddedFilesDialog(QWidget *parent, const KPDFDocument *document);
	
	private slots:
		void saveFile();
	
	private:
		QTreeWidget *m_tw;
		QHash<QTreeWidgetItem *, EmbeddedFile*> m_files;
};

#endif
