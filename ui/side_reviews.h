/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_SIDE_REVIEWS_H_
#define _OKULAR_SIDE_REVIEWS_H_

#include <qwidget.h>
#include <qvector.h>
#include "core/observer.h"

namespace Okular {
class Annotation;
class Document;
class Page;
}

class QToolBar;
class QTreeWidget;
class QTreeWidgetItem;
class KTreeWidgetSearchLine;
class QTimer;

/**
 * @short ...
 */
class Reviews : public QWidget, public Okular::DocumentObserver
{
    Q_OBJECT
    public:
        Reviews( QWidget * parent, Okular::Document * document );

        // [INHERITED] from DocumentObserver
        uint observerId() const { return REVIEWS_ID; }
        void notifySetup( const QVector< Okular::Page * > & pages, bool );
        void notifyViewportChanged( bool smoothMove );
        void notifyPageChanged( int pageNumber, int changedFlags );

    public Q_SLOTS:
        void slotPageEnabled( bool );
        void slotAuthorEnabled( bool );
        void slotCurrentPageOnly( bool );
        void slotUpdateListView();

    Q_SIGNALS:
        void setAnnotationWindow( Okular::Annotation *annotation );
        void removeAnnotationWindow( Okular::Annotation *annotation );

    private Q_SLOTS:
        void itemActivated( QTreeWidgetItem *, int );
        void itemEntered( QTreeWidgetItem *, int );
        void contextMenuRequested( const QPoint &pos );

    private:
        // add all annotations of a page to the listView taking care of grouping
        void addContents( const Okular::Page * page );
        // delay an update to the listView
        void requestListViewUpdate( int delayms = 200 );

        // data fields (GUI)
        QToolBar * m_toolBar1;
        QToolBar * m_toolBar2;
        QTreeWidget * m_listView;
        KTreeWidgetSearchLine * m_searchLine;
        // internal storage
        Okular::Document * m_document;
        QVector< Okular::Page * > m_pages;
        QTimer * m_delayTimer;
        int m_currentPage;
};

#endif
