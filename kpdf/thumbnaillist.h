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

#include <qscrollview.h>
#include <qvaluevector.h>
#include "document.h"

class Thumbnail;

class ThumbnailList : public QScrollView, public KPDFDocumentObserver
{
Q_OBJECT
	public:
		ThumbnailList(QWidget *parent, KPDFDocument *document);

		// inherited as a KPDFDocumentObserver
		void pageSetup( const QValueList<int> & pages );
		void pageSetCurrent( int pageNumber, float position );
		void notifyThumbnailChanged( int pageNumber );

	public slots:
		void slotRequestThumbnails( int newContentsX = -1, int newContentsY = -1 );

	protected:
		void viewportResizeEvent( QResizeEvent * );
		void contentsMousePressEvent( QMouseEvent * );
		void keyPressEvent( QKeyEvent * e );

	private:
		KPDFDocument *m_document;
		Thumbnail *m_selected;
		int m_oldWidth;
		int m_oldHeight;
		QValueVector<Thumbnail *> thumbnails;
};

#endif
