/***************************************************************************
 *   Copyright (C) 2015 by Saheb Preet Singh <saheb.preet@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "layers.h"
#include "settings.h"

// qt/kde includes
#include <KLocalizedString>
#include <KTitleWidget>
#include <QHeaderView>
#include <QTreeView>
#include <QVBoxLayout>

#include <kwidgetsaddons_version.h>

// local includes
#include "core/document.h"
#include "ktreeviewsearchline.h"
#include "ui/pageview.h"

Layers::Layers(QWidget *parent, Okular::Document *document)
    : QWidget(parent)
    , m_document(document)
{
    QVBoxLayout *const mainlay = new QVBoxLayout(this);
    mainlay->setSpacing(6);

    m_document->addObserver(this);

    KTitleWidget *titleWidget = new KTitleWidget(this);
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 53, 0)
    titleWidget->setLevel(2);
#endif
    titleWidget->setText(i18n("Layers"));
    mainlay->addWidget(titleWidget);
    mainlay->setAlignment(titleWidget, Qt::AlignHCenter);
    m_searchLine = new KTreeViewSearchLine(this);
    mainlay->addWidget(m_searchLine);
    m_searchLine->setCaseSensitivity(Okular::Settings::self()->layersSearchCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive);
    m_searchLine->setRegularExpression(Okular::Settings::self()->layersSearchRegularExpression());
    connect(m_searchLine, &KTreeViewSearchLine::searchOptionsChanged, this, &Layers::saveSearchOptions);

    m_treeView = new QTreeView(this);
    mainlay->addWidget(m_treeView);

    m_treeView->setSortingEnabled(false);
    m_treeView->setRootIsDecorated(true);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->header()->hide();
}

Layers::~Layers()
{
    m_document->removeObserver(this);
}

void Layers::notifySetup(const QVector<Okular::Page *> & /*pages*/, int /*setupFlags*/)
{
    QAbstractItemModel *layersModel = m_document->layersModel();

    if (layersModel) {
        m_treeView->setModel(layersModel);
        m_searchLine->setTreeView(m_treeView);
        emit hasLayers(true);
        connect(layersModel, &QAbstractItemModel::dataChanged, m_document, &Okular::Document::reloadDocument);
        connect(layersModel, &QAbstractItemModel::dataChanged, m_pageView, &PageView::reloadForms);
    } else {
        emit hasLayers(false);
    }
}

void Layers::setPageView(PageView *pageView)
{
    m_pageView = pageView;
}

void Layers::saveSearchOptions()
{
    Okular::Settings::setLayersSearchRegularExpression(m_searchLine->regularExpression());
    Okular::Settings::setLayersSearchCaseSensitive(m_searchLine->caseSensitivity() == Qt::CaseSensitive ? true : false);
    Okular::Settings::self()->save();
}
