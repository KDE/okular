/*
    SPDX-FileCopyrightText: 2025 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "welcomeitem.h"

#include <KConfigGroup>
#include <KSharedConfig>

WelcomeItem::WelcomeItem(QObject *parent)
    : QObject(parent)
    , m_recentItemsModel(new RecentItemsModel)
{
    m_recentItemsModel->loadEntries(KSharedConfig::openConfig()->group(QStringLiteral("Recent Files")));
}

WelcomeItem::~WelcomeItem()
{
    delete m_recentItemsModel;
}

RecentItemsModel *WelcomeItem::recentItemsModel() const
{
    return m_recentItemsModel;
}

void WelcomeItem::urlOpened(const QUrl &url)
{
    auto cg = KSharedConfig::openConfig()->group(QStringLiteral("Recent Files"));
    cg.deleteGroup();

    // Note: entries are indexed starting with 1, see krecentfilesaction.cpp in kconfigwidgets
    // last entry is our current file
    const int numRows = m_recentItemsModel->rowCount();
    for (int r = 1; r <= numRows; r++) {
        // model index row starts at zero
        const QUrl _url = m_recentItemsModel->data(m_recentItemsModel->index((r - 1), 0), RecentItemsModel::UrlRole).toUrl();
        const QString &shortName = _url.fileName();
        cg.writePathEntry(QStringLiteral("File%1").arg(r), _url.toDisplayString(QUrl::PreferLocalFile));
        cg.writePathEntry(QStringLiteral("Name%1").arg(r), shortName);
    }
    const QString &shortName = url.fileName();
    cg.writePathEntry(QStringLiteral("File%1").arg(numRows + 1), url.toDisplayString(QUrl::PreferLocalFile));
    cg.writePathEntry(QStringLiteral("Name%1").arg(numRows + 1), shortName);
}
