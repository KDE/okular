/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef THUMBNAILLIST_H
#define THUMBNAILLIST_H

#include <qtable.h>
#include "document.h"

class QImage;

class PDFDoc;

class ThumbnailGenerator;

class ThumbnailList : public QTable, public KPDFDocumentObserver
{
Q_OBJECT
	public:
		ThumbnailList(QWidget *parent, QMutex *docMutex);
		~ThumbnailList();

		// inherited as KPDFDocumentObserver
		void pageSetup( const QValueList<int> & pages )
		{
			// TODO use a qvaluelist<int> to pass aspect ratio?
			// TODO move it the thumbnail list?
			setPages( pages.count(), 2 );
			//generateThumbnails(d->pdfdoc);
		}
		void pageSetCurrent( int /*pageNumber*/, float /*position*/ )
		{
			//setCurrentThumbnail(m_currentPage);
		}

		void setCurrentThumbnail(int i);
		void setPages(int i, double ar);
		
		void generateThumbnails(PDFDoc *doc);
		void stopThumbnailGeneration();
	
	protected slots:
		void customEvent(QCustomEvent *e);
	
	private slots:
		void changeSelected(int i);
		void emitClicked(int i);
		
	signals:
		void clicked(int);
	
	protected:
		void viewportResizeEvent(QResizeEvent *);
	
	private:
		void generateNextThumbnail();
		void resizeThumbnails();
		void setThumbnail(int i, const QImage *thumbnail);
		
		double m_ar;
		int m_selected;
		int m_heightLimit;
		int m_nextThumbnail;
		ThumbnailGenerator *m_tg;
		PDFDoc *m_doc;
		QMutex *m_docMutex;
		bool m_ignoreNext;
};

#endif
