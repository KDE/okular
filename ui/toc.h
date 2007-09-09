/***************************************************************************
 *   Copyright (C) 2004-2006 by Albert Astals Cid <tsdgeos@terra.es>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_TOC_H_
#define _OKULAR_TOC_H_

#include <qwidget.h>
#include "core/observer.h"

class QDomNode;
class QModelIndex;
class QTreeView;
class KTreeViewSearchLine;
class TOCModel;

namespace Okular {
class Document;
}

class TOC : public QWidget, public Okular::DocumentObserver
{
Q_OBJECT
    public:
        TOC(QWidget *parent, Okular::Document *document);
        ~TOC();

        // inherited from DocumentObserver
        uint observerId() const;
        void notifySetup( const QVector< Okular::Page * > & pages, int setupFlags );
        void notifyViewportChanged( bool smoothMove );

        void reparseConfig();

    signals:
        void hasTOC(bool has);

    private slots:
        void slotExecuted( const QModelIndex & );

    private:
        Okular::Document *m_document;
        QTreeView *m_treeView;
        KTreeViewSearchLine *m_searchLine;
        TOCModel *m_model;
        int m_currentPage;
};

#endif
