/*
    SPDX-FileCopyrightText: 2021 Jiří Wolker <woljiri@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "recentitemsmodel.h"

#include <QFile>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QIcon>

#include <KConfigGroup>

RecentItemsModel::RecentItemsModel()
{
}

RecentItemsModel::~RecentItemsModel()
{
}

void RecentItemsModel::loadEntries(const KConfigGroup &cg)
{
    clearEntries();

    // Based on implementation of KRecentFilesAction::loadEntries.

    QString key;
    QString value;
    QString nameKey;
    QString nameValue;
    QUrl url;

    // read file list
    for (int i = 1; i <= maxItems(); i++) {
        key = QStringLiteral("File%1").arg(i);
        value = cg.readPathEntry(key, QString());
        if (value.isEmpty()) {
            continue;
        }
        url = QUrl::fromUserInput(value);

        // Don't restore if file doesn't exist anymore
        if (url.isLocalFile() && !QFile::exists(url.toLocalFile())) {
            continue;
        }

        nameKey = QStringLiteral("Name%1").arg(i);
        nameValue = cg.readPathEntry(nameKey, url.fileName());
        m_recentItems.append(RecentItem {nameValue, url});
    }

    Q_EMIT layoutChanged();
}

void RecentItemsModel::clearEntries()
{
    m_recentItems.clear();

    Q_EMIT layoutChanged();
}

int RecentItemsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_recentItems.size();
}

QVariant RecentItemsModel::data(const QModelIndex &index, int role) const
{
    const RecentItemsModel::RecentItem *item = getItem(index);

    if (item != nullptr) {
        switch (role) {
        case Qt::ItemDataRole::DisplayRole:
            if (item->name.isEmpty()) {
                if (item->url.isLocalFile()) {
                    return item->url.toLocalFile();
                } else {
                    return item->url.toString();
                }
            } else {
                return item->name;
            }

        case Qt::ItemDataRole::ToolTipRole:
            if (item->url.isLocalFile()) {
                return item->url.toLocalFile();
            } else {
                return item->url.toString();
            }

        case Qt::ItemDataRole::DecorationRole:
            if (item->url.isLocalFile()) {
                return m_iconProvider.icon(QFileInfo(item->url.toLocalFile()));
            } else {
                // Fallback icon for remote files.
                return QIcon::fromTheme(QStringLiteral("network-server"));
            }

        default:
            return QVariant();
        }
    } else {
        return QVariant();
    }
}

RecentItemsModel::RecentItem const *RecentItemsModel::getItem(int index) const
{
    if (m_recentItems.size() > index && index >= 0) {
        // We reverse the order of items.
        return &m_recentItems[m_recentItems.size() - index - 1];
    } else {
        return nullptr;
    }
}

RecentItemsModel::RecentItem const *RecentItemsModel::getItem(const QModelIndex &index) const
{
    return getItem(index.row());
}
