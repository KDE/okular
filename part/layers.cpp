/*
    SPDX-FileCopyrightText: 2015 Saheb Preet Singh <saheb.preet@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
#include "pageview.h"

Layers::Layers(QWidget *parent, Okular::Document *document)
    : QWidget(parent)
    , m_document(document)
{
    auto mainlay = new QVBoxLayout(this);
    mainlay->setSpacing(0);
    mainlay->setContentsMargins({});

    m_document->addObserver(this);

    KTitleWidget *titleWidget = new KTitleWidget(this);
    titleWidget->setLevel(4);
    titleWidget->setText(i18n("Layers"));
    titleWidget->setContentsMargins(0, titleWidget->style()->pixelMetric(QStyle::PM_LayoutTopMargin), 0, titleWidget->style()->pixelMetric(QStyle::PM_LayoutBottomMargin));
    mainlay->addWidget(titleWidget);
    mainlay->setAlignment(titleWidget, Qt::AlignHCenter);

    auto lineContainer = new QWidget(this);
    auto containerLayout = new QVBoxLayout(lineContainer);

    m_searchLine = new KTreeViewSearchLine(lineContainer);
    m_searchLine->setPlaceholderText(i18n("Search..."));
    m_searchLine->setCaseSensitivity(Okular::Settings::self()->layersSearchCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive);
    m_searchLine->setRegularExpression(Okular::Settings::self()->layersSearchRegularExpression());
    connect(m_searchLine, &KTreeViewSearchLine::searchOptionsChanged, this, &Layers::saveSearchOptions);
    containerLayout->addWidget(m_searchLine);
    mainlay->addWidget(lineContainer);

    m_treeView = new QTreeView(this);
    mainlay->addWidget(m_treeView);

    m_treeView->setSortingEnabled(false);
    m_treeView->setRootIsDecorated(true);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setProperty("_breeze_borders_sides", QVariant::fromValue(QFlags{Qt::BottomEdge | Qt::TopEdge}));
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
        Q_EMIT hasLayers(true);
        connect(layersModel, &QAbstractItemModel::dataChanged, m_document, &Okular::Document::reloadDocument);
        connect(layersModel, &QAbstractItemModel::dataChanged, m_pageView, &PageView::reloadForms);
    } else {
        Q_EMIT hasLayers(false);
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
