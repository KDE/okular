/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_THUMBNAILLIST_H_
#define _KPDF_THUMBNAILLIST_H_

#include <qscrollview.h>
#include <qvaluevector.h>
#include <qvbox.h>
#include <ktoolbar.h>
#include "core/observer.h"

class QTimer;
class KActionCollection;

class KPDFDocument;
class ThumbnailWidget;

/**
 * @short A scrollview that displays pages pixmaps previews (aka thumbnails).
 *
 * ...
 */
class ThumbnailList : public QScrollView, public DocumentObserver
{
Q_OBJECT
	public:
		ThumbnailList(QWidget *parent, KPDFDocument *document);
		~ThumbnailList();

        // inherited: return thumbnails observer id
        uint observerId() const { return THUMBNAILS_ID; }
        // inherited: create thumbnails ( inherited as a DocumentObserver )
        void notifySetup( const QValueVector< KPDFPage * > & pages, bool documentChanged );
        // inherited: hilihght current thumbnail ( inherited as DocumentObserver )
        void notifyViewportChanged( bool smoothMove );
        // inherited: redraw thumbnail ( inherited as DocumentObserver )
        void notifyPageChanged( int pageNumber, int changedFlags );
        // inherited: request all visible pixmap (due to a global shange or so..)
        void notifyContentsCleared( int changedFlags );
        // inherited: tell if pixmap is hidden and can be unloaded
        bool canUnloadPixmap( int pageNumber );

        // redraw visible widgets (useful for refreshing contents...)
        void updateWidgets();

        // called by ThumbnailWidgets to send (forward) rightClick signals
        void forwardRightClick( const KPDFPage *, const QPoint & );
        // called by ThumbnailWidgets to get the overlay bookmark pixmap
        const QPixmap * getBookmarkOverlay() const;

    public slots:
        // these are connected to ThumbnailController buttons
        void slotFilterBookmarks( bool filterOn );

	protected:
		// scroll up/down the view
		void keyPressEvent( QKeyEvent * e );

		// select a thumbnail by clicking on it
		void contentsMousePressEvent( QMouseEvent * );

		// resize thumbnails to fit the width
		void viewportResizeEvent( QResizeEvent * );

		// file drop related events (an url may be dropped even here)
		void dragEnterEvent( QDragEnterEvent* );
		void dropEvent( QDropEvent* );

	signals:
		void urlDropped( const KURL& );
		void rightClick( const KPDFPage *, const QPoint & );

	private:
		void delayedRequestVisiblePixmaps( int delayMs = 0 );
		KPDFDocument *m_document;
		ThumbnailWidget *m_selected;
		QTimer *m_delayTimer;
		QPixmap *m_bookmarkOverlay;
		QValueVector<ThumbnailWidget *> m_thumbnails;
		QValueList<ThumbnailWidget *> m_visibleThumbnails;
		int m_vectorIndex;

	private slots:
		// make requests for generating pixmaps for visible thumbnails
		void slotRequestVisiblePixmaps( int newContentsX = -1, int newContentsY = -1 );
		// delay timeout: resize overlays and requests pixmaps
		void slotDelayTimeout();
};

/**
 * @short A vertical boxed container with zero size hint (for insertion on left toolbox)
 */
class ThumbnailsBox : public QVBox
{
	public:
		ThumbnailsBox( QWidget * parent ) : QVBox( parent ) {};
		QSize sizeHint() const { return QSize(); }
};

/**
 * @short A toolbar thar set ThumbnailList properties when clicking on items
 *
 * This class is the small tolbar that resides in the bottom of the
 * ThumbnailsBox container (below ThumbnailList and the SearchLine) and
 * emits signals whenever a button is pressed. A click action results
 * in invoking some method (or slot) in ThumbnailList.
 */
class ThumbnailController : public KToolBar
{
    public:
        ThumbnailController( QWidget * parent, ThumbnailList * thumbnailList );
};

#endif
