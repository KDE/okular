/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef THUMBNAILGENERATOR_H
#define THUMBNAILGENERATOR_H

#include <qthread.h> 

#include "QOutputDevPixmap.h"

class QMutex;

class PDFDoc;

class ThumbnailGenerator : public QThread
{
	public:
		ThumbnailGenerator(PDFDoc *doc, QMutex *docMutex, int page, double ppp, QObject *o);
		
		int getPage() const;

	protected:
		void run();
	
	private:
		PDFDoc *m_doc;
		QMutex *m_docMutex;
		int m_page;
		QObject *m_o;
		double m_ppp;
};

#endif
