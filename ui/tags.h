/***************************************************************************
 *   Copyright (C) 2015 by Saheb Preet Singh <saheb.preet@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_TAGS_H_
#define _OKULAR_TAGS_H_

#include <qwidget.h>

#include "core/observer.h"
#include "okular_part_export.h"

class QModelIndex;
class QTreeView;
class KTreeViewSearchLine;

namespace Okular {
    class Document;
}

class OKULAR_PART_EXPORT Tags : public QWidget, public Okular::DocumentObserver
{
    Q_OBJECT

    public:
	Tags( QWidget *parent, Okular::Document *document );
	~Tags();

        // inherited from DocumentObserver
        void notifySetup( const QVector< Okular::Page * > & pages, int setupFlags );

    signals:
	void hasTags(bool has);

    private slots:
        void saveSearchOptions();
	void highlightDocument( QModelIndex index );

    private:

        Okular::Document *m_document;
        QTreeView *m_treeView;
        KTreeViewSearchLine *m_searchLine;
};

#endif
