/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_THUMBNAILLIST_H_
#define _OKULAR_THUMBNAILLIST_H_

#include <qscrollarea.h>

#include <kvbox.h>
#include <qtoolbar.h>

#include "core/area.h"
#include "core/observer.h"

class QTimer;
class KUrl;
class ThumbnailWidget;

namespace Okular {
class Document;
}

/**
 * @short A scrollview that displays pages pixmaps previews (aka thumbnails).
 *
 * ...
 */
class ThumbnailList : public QScrollArea, public Okular::DocumentObserver
{
Q_OBJECT
    public:
        ThumbnailList(QWidget *parent, Okular::Document *document);
        ~ThumbnailList();

        // inherited: return thumbnails observer id
        uint observerId() const { return THUMBNAILS_ID; }
        // inherited: create thumbnails ( inherited as a DocumentObserver )
        void notifySetup( const QVector< Okular::Page * > & pages, int setupFlags );
        // inherited: hilihght current thumbnail ( inherited as DocumentObserver )
        void notifyViewportChanged( bool smoothMove );
        // inherited: redraw thumbnail ( inherited as DocumentObserver )
        void notifyPageChanged( int pageNumber, int changedFlags );
        // inherited: request all visible pixmap (due to a global shange or so..)
        void notifyContentsCleared( int changedFlags );
        // inherited: the visible areas of the page have changed
        void notifyVisibleRectsChanged();
        // inherited: tell if pixmap is hidden and can be unloaded
        bool canUnloadPixmap( int pageNumber ) const;

        // redraw visible widgets (useful for refreshing contents...)
        void updateWidgets();

        // called by ThumbnailWidgets to send (forward) the mouse click signals
        void forwardClick( const Okular::Page *, const QPoint &, Qt::MouseButton );
        // called by ThumbnailWidgets to send (forward) the mouse move signals
        void forwardTrack( const Okular::Page *, const QPoint &, const QPoint & );
        // called by ThumbnailWidgets to send (forward) the mouse zoom signals
        void forwardZoom( const Okular::Page *, int );
        // called by ThumbnailWidgets to get the overlay bookmark pixmap
        const QPixmap * getBookmarkOverlay() const;

    public slots:
        // these are connected to ThumbnailController buttons
        void slotFilterBookmarks( bool filterOn );

    protected:
        // scroll up/down the view
        void keyPressEvent( QKeyEvent * e );

        // resize thumbnails to fit the width
        void viewportResizeEvent( QResizeEvent * );
        // catch the viewport event and filter them if necessary
        bool viewportEvent( QEvent * );

        // file drop related events (an url may be dropped even here)
        void dragEnterEvent( QDragEnterEvent* );
        void dropEvent( QDropEvent* );

    signals:
        void urlDropped( const KUrl& );
        void rightClick( const Okular::Page *, const QPoint & );

    private:
        void delayedRequestVisiblePixmaps( int delayMs = 0 );
        Okular::Document *m_document;
        ThumbnailWidget *m_selected;
        QTimer *m_delayTimer;
        QPixmap *m_bookmarkOverlay;
        QVector<ThumbnailWidget *> m_thumbnails;
        QList<ThumbnailWidget *> m_visibleThumbnails;
        int m_vectorIndex;
        QWidget *m_pagesWidget;

    private slots:
        // make requests for generating pixmaps for visible thumbnails
        void slotRequestVisiblePixmaps( int newContentsY = -1 );
        // delay timeout: resize overlays and requests pixmaps
        void slotDelayTimeout();
};

/**
 * @short A vertical boxed container with zero size hint (for insertion on left toolbox)
 */
class ThumbnailsBox : public KVBox
{
    public:
        ThumbnailsBox( QWidget * parent ) : KVBox( parent ) {}
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
class ThumbnailController : public QToolBar
{
    public:
        ThumbnailController( QWidget * parent, ThumbnailList * thumbnailList );
};

#endif
