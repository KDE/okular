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

#include "core/document.h"

class TOC : public KListView, public KPDFDocumentObserver
{
Q_OBJECT
    public:
        TOC(QWidget *parent, KPDFDocument *document);

        // inherited from KPDFDocumentObserver
        uint observerId() const;
        void pageSetup( const QValueVector<KPDFPage*> & pages, bool documentChanged );

    signals:
        void hasTOC(bool has);

    private slots:
        void slotExecuted(QListViewItem *i);

    private:
        void addChildren( const QDomNode & parentNode, KListViewItem * parentItem = 0 );
        KPDFDocument *m_document;
};

#endif
