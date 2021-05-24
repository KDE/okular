/*
    SPDX-FileCopyrightText: 2004-2006 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "toc.h"

// qt/kde includes
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QLayout>
#include <QTreeView>
#include <qdom.h>

#include <KLineEdit>
#include <KLocalizedString>
#include <KTitleWidget>

#include <kwidgetsaddons_version.h>

// local includes
#include "core/action.h"
#include "ktreeviewsearchline.h"
#include "pageitemdelegate.h"
#include "settings.h"
#include "tocmodel.h"

TOC::TOC(QWidget *parent, Okular::Document *document)
    : QWidget(parent)
    , m_document(document)
{
    QVBoxLayout *mainlay = new QVBoxLayout(this);
    mainlay->setSpacing(6);

    KTitleWidget *titleWidget = new KTitleWidget(this);
    titleWidget->setLevel(2);
    titleWidget->setText(i18n("Contents"));
    mainlay->addWidget(titleWidget);
    mainlay->setAlignment(titleWidget, Qt::AlignHCenter);
    m_searchLine = new KTreeViewSearchLine(this);
    mainlay->addWidget(m_searchLine);
    m_searchLine->setPlaceholderText(i18n("Search..."));
    m_searchLine->setCaseSensitivity(Okular::Settings::self()->contentsSearchCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive);
    m_searchLine->setRegularExpression(Okular::Settings::self()->contentsSearchRegularExpression());
    connect(m_searchLine, &KTreeViewSearchLine::searchOptionsChanged, this, &TOC::saveSearchOptions);

    m_treeView = new QTreeView(this);
    mainlay->addWidget(m_treeView);
    m_model = new TOCModel(document, m_treeView);
    m_treeView->setModel(m_model);
    m_treeView->setSortingEnabled(false);
    m_treeView->setRootIsDecorated(true);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setItemDelegate(new PageItemDelegate(m_treeView));
    m_treeView->header()->hide();
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(m_treeView, &QTreeView::clicked, this, &TOC::slotExecuted);
    connect(m_treeView, &QTreeView::activated, this, &TOC::slotExecuted);
    m_searchLine->setTreeView(m_treeView);
}

TOC::~TOC()
{
    m_document->removeObserver(this);
}

void TOC::notifySetup(const QVector<Okular::Page *> & /*pages*/, int setupFlags)
{
    if (!(setupFlags & Okular::DocumentObserver::DocumentChanged))
        return;

    // clear contents
    m_model->clear();

    // request synopsis description (is a dom tree)
    const Okular::DocumentSynopsis *syn = m_document->documentSynopsis();
    if (!syn) {
        if (m_document->isOpened()) {
            // Make sure we clear the reload old model data
            m_model->setOldModelData(nullptr, QVector<QModelIndex>());
        }
        emit hasTOC(false);
        return;
    }

    m_model->fill(syn);
    emit hasTOC(!m_model->isEmpty());
}

void TOC::notifyCurrentPageChanged(int, int)
{
    m_model->setCurrentViewport(m_document->viewport());
}

void TOC::prepareForReload()
{
    if (m_model->isEmpty())
        return;

    const QVector<QModelIndex> list = expandedNodes();
    TOCModel *m = m_model;
    m_model = new TOCModel(m_document, m_treeView);
    m_model->setOldModelData(m, list);
    m->setParent(nullptr);
}

void TOC::rollbackReload()
{
    if (!m_model->hasOldModelData())
        return;

    TOCModel *m = m_model;
    m_model = m->clearOldModelData();
    m_model->setParent(m_treeView);
    delete m;
}

void TOC::finishReload()
{
    m_treeView->setModel(m_model);
    m_model->setParent(m_treeView);
}

QVector<QModelIndex> TOC::expandedNodes(const QModelIndex &parent) const
{
    QVector<QModelIndex> list;
    for (int i = 0; i < m_model->rowCount(parent); i++) {
        const QModelIndex index = m_model->index(i, 0, parent);
        if (m_treeView->isExpanded(index)) {
            list << index;
        }
        if (m_model->hasChildren(index)) {
            list << expandedNodes(index);
        }
    }
    return list;
}

void TOC::reparseConfig()
{
    m_searchLine->setCaseSensitivity(Okular::Settings::contentsSearchCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive);
    m_searchLine->setRegularExpression(Okular::Settings::contentsSearchRegularExpression());
    m_treeView->update();
}

void TOC::slotExecuted(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    QString url = m_model->urlForIndex(index);
    if (!url.isEmpty()) {
        Okular::BrowseAction action(QUrl::fromLocalFile(url));
        m_document->processAction(&action);
        return;
    }

    QString externalFileName = m_model->externalFileNameForIndex(index);
    Okular::DocumentViewport viewport = m_model->viewportForIndex(index);
    if (!externalFileName.isEmpty()) {
        Okular::GotoAction action(externalFileName, viewport);
        m_document->processAction(&action);
    } else if (viewport.isValid()) {
        m_document->setViewport(viewport);
    }
}

void TOC::saveSearchOptions()
{
    Okular::Settings::setContentsSearchRegularExpression(m_searchLine->regularExpression());
    Okular::Settings::setContentsSearchCaseSensitive(m_searchLine->caseSensitivity() == Qt::CaseSensitive ? true : false);
    Okular::Settings::self()->save();
}

void TOC::contextMenuEvent(QContextMenuEvent *e)
{
    QModelIndex index = m_treeView->currentIndex();
    if (!index.isValid())
        return;

    Okular::DocumentViewport viewport = m_model->viewportForIndex(index);

    emit rightClick(viewport, e->globalPos(), m_model->data(index).toString());
}

void TOC::expandRecursively()
{
    QList<QModelIndex> worklist = {m_treeView->currentIndex()};
    if (!worklist[0].isValid()) {
        return;
    }
    while (!worklist.isEmpty()) {
        QModelIndex index = worklist.takeLast();
        m_treeView->expand(index);
        for (int i = 0; i < m_model->rowCount(index); i++) {
            worklist += m_model->index(i, 0, index);
        }
    }
}

void TOC::collapseRecursively()
{
    QList<QModelIndex> worklist = {m_treeView->currentIndex()};
    if (!worklist[0].isValid()) {
        return;
    }
    while (!worklist.isEmpty()) {
        QModelIndex index = worklist.takeLast();
        m_treeView->collapse(index);
        for (int i = 0; i < m_model->rowCount(index); i++) {
            worklist += m_model->index(i, 0, index);
        }
    }
}

void TOC::expandAll()
{
    m_treeView->expandAll();
}

void TOC::collapseAll()
{
    m_treeView->collapseAll();
}
