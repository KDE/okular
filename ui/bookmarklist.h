/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef BOOKMARKLIST_H
#define BOOKMARKLIST_H

#include <qwidget.h>

#include "core/observer.h"

class QAction;
class QTreeWidget;
class QTreeWidgetItem;
class KTreeWidgetSearchLine;
class KUrl;
class BookmarkItem;

namespace Okular {
class Document;
}

class BookmarkList : public QWidget, public Okular::DocumentObserver
{
    Q_OBJECT

    public:
        BookmarkList( Okular::Document *document, QWidget *parent = 0 );
        ~BookmarkList();

        // inherited from DocumentObserver
        uint observerId() const;
        void notifySetup( const QVector< Okular::Page * > & pages, bool documentChanged );
        void notifyPageChanged( int pageNumber, int changedFlags );

    signals:
        void openUrl( const KUrl& );

    private slots:
        void slotFilterBookmarks( bool );
        void slotExecuted( QTreeWidgetItem * item );
        void slotChanged( QTreeWidgetItem * item );
        void slotContextMenu( const QPoint& p );

    private:
        void rebuildTree( bool filter );
        void goTo( BookmarkItem * item );

        Okular::Document * m_document;
        QTreeWidget * m_tree;
        KTreeWidgetSearchLine * m_searchLine;
        QAction * m_showBoomarkOnlyAction;
};

#endif
