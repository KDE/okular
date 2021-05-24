/*
    SPDX-FileCopyrightText: 2004-2006 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_TOC_H_
#define _OKULAR_TOC_H_

#include "core/document.h"
#include "core/observer.h"
#include <QModelIndex>
#include <qwidget.h>

#include "okularpart_export.h"

class QModelIndex;
class QTreeView;
class KTreeViewSearchLine;
class TOCModel;

namespace Okular
{
class Document;
class PartTest;
}

class OKULARPART_EXPORT TOC : public QWidget, public Okular::DocumentObserver
{
    Q_OBJECT
    friend class Okular::PartTest;

public:
    TOC(QWidget *parent, Okular::Document *document);
    ~TOC() override;

    // inherited from DocumentObserver
    void notifySetup(const QVector<Okular::Page *> &pages, int setupFlags) override;
    void notifyCurrentPageChanged(int previous, int current) override;

    void reparseConfig();

    void prepareForReload();
    void rollbackReload();
    void finishReload();

public Q_SLOTS:
    void expandRecursively();
    void collapseRecursively();
    void expandAll();
    void collapseAll();

Q_SIGNALS:
    void hasTOC(bool has);
    void rightClick(const Okular::DocumentViewport &, const QPoint, const QString &);

private Q_SLOTS:
    void slotExecuted(const QModelIndex &);
    void saveSearchOptions();

protected:
    void contextMenuEvent(QContextMenuEvent *e) override;

private:
    QVector<QModelIndex> expandedNodes(const QModelIndex &parent = QModelIndex()) const;

    Okular::Document *m_document;
    QTreeView *m_treeView;
    KTreeViewSearchLine *m_searchLine;
    TOCModel *m_model;
};

#endif
