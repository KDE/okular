/***************************************************************************
 *   Copyright (C) 2004-2006 by Albert Astals Cid <tsdgeos@terra.es>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_TOC_H_
#define _KPDF_TOC_H_

#include <qdom.h>
#include <k3listview.h>
#include "core/document.h"
#include "core/observer.h"

class KPDFDocument;

class TOC : public K3ListView, public DocumentObserver
{
Q_OBJECT
    public:
        TOC(QWidget *parent, KPDFDocument *document);
        ~TOC();

        // inherited from DocumentObserver
        uint observerId() const;
        void notifySetup( const QVector< KPDFPage * > & pages, bool documentChanged );

    signals:
        void hasTOC(bool has);

    private slots:
        void slotExecuted(Q3ListViewItem *i);

    private:
        void addChildren( const QDomNode & parentNode, K3ListViewItem * parentItem = 0 );
        DocumentViewport getViewport( const QDomElement &e ) const;
        KPDFDocument *m_document;
};

#endif
