/***************************************************************************
 *   Copyright (C) 2004-2006 by Albert Astals Cid <aacid@kde.org>          *
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
#include <QModelIndex>

#include "okular_part_export.h"

class QDomNode;
class QModelIndex;
class QTreeView;
class KTreeViewSearchLine;
class TOCModel;

namespace Okular {
class Document;
class PartTest;
}

class OKULAR_PART_EXPORT TOC : public QWidget, public Okular::DocumentObserver
{
Q_OBJECT
    friend class Okular::PartTest;
    
    public:
        TOC(QWidget *parent, Okular::Document *document);
        ~TOC();

        // inherited from DocumentObserver
        void notifySetup( const QVector< Okular::Page * > & pages, int setupFlags );
        void notifyCurrentPageChanged( int previous, int current );

        void reparseConfig();

        void prepareForReload();
        void rollbackReload();
        void finishReload();

    signals:
        void hasTOC(bool has);

    private slots:
        void slotExecuted( const QModelIndex & );
        void saveSearchOptions();

    private:
        QVector<QModelIndex> expandedNodes( const QModelIndex & parent=QModelIndex() ) const;

        Okular::Document *m_document;
        QTreeView *m_treeView;
        KTreeViewSearchLine *m_searchLine;
        TOCModel *m_model;
};

#endif
