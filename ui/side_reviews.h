/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_SIDE_REVIEWS_H_
#define _KPDF_SIDE_REVIEWS_H_

#include <qwidget.h>
#include <qvector.h>
#include "core/observer.h"

class KPDFDocument;
class KPDFPage;
class QToolBar;
class K3ListView;
class K3ListViewSearchLine;
class QTimer;

/**
 * @short ...
 */
class Reviews : public QWidget, public DocumentObserver
{
    Q_OBJECT
    public:
        Reviews( QWidget * parent, KPDFDocument * document );

        // [INHERITED] from DocumentObserver
        uint observerId() const { return REVIEWS_ID; }
        void notifySetup( const QVector< KPDFPage * > & pages, bool );
        void notifyViewportChanged( bool smoothMove );
        void notifyPageChanged( int pageNumber, int changedFlags );

    public slots:
        void slotPageEnabled( bool );
        void slotAuthorEnabled( bool );
        void slotCurrentPageOnly( bool );
        void slotUpdateListView();

    private:
        // add all annotations of a page to the listView taking care of grouping
        void addContents( const KPDFPage * page );
        // delay an update to the listView
        void requestListViewUpdate( int delayms = 200 );

        // data fields (GUI)
        QToolBar * m_toolBar1;
        QToolBar * m_toolBar2;
        K3ListView * m_listView;
        K3ListViewSearchLine * m_searchLine;
        // internal storage
        KPDFDocument * m_document;
        QVector< KPDFPage * > m_pages;
        QTimer * m_delayTimer;
        int m_currentPage;
};

#endif
