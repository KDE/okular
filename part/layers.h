/***************************************************************************
 *   Copyright (C) 2015 by Saheb Preet Singh <saheb.preet@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_LAYERS_H_
#define _OKULAR_LAYERS_H_

#include "core/observer.h"
#include <qwidget.h>

#include "okularpart_export.h"

class PageView;
class QTreeView;
class KTreeViewSearchLine;

namespace Okular
{
class Document;
class PartTest;
}

class OKULARPART_EXPORT Layers : public QWidget, public Okular::DocumentObserver
{
    Q_OBJECT
    friend class Okular::PartTest;

public:
    Layers(QWidget *parent, Okular::Document *document);
    ~Layers() override;

    // inherited from DocumentObserver
    void notifySetup(const QVector<Okular::Page *> &pages, int setupFlags) override;

    void setPageView(PageView *pageView);

Q_SIGNALS:
    void hasLayers(bool has);

private Q_SLOTS:
    void saveSearchOptions();

private:
    Okular::Document *m_document;
    QTreeView *m_treeView;
    KTreeViewSearchLine *m_searchLine;
    PageView *m_pageView;
};

#endif
