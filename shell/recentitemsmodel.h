/*
    SPDX-FileCopyrightText: 2021 Jiří Wolker <woljiri@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef RECENTITEMSMODEL_H
#define RECENTITEMSMODEL_H

#include <QAbstractListModel>
#include <QFileIconProvider>
#include <QList>
#include <QModelIndex>
#include <QString>
#include <QUrl>

class KConfigGroup;

/**
 * @todo write docs
 */
class RecentItemsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    struct RecentItem {
        QString name;
        QUrl url;
    };

    /**
     * Default constructor
     */
    RecentItemsModel();

    /**
     * Destructor
     */
    ~RecentItemsModel() override;

    void loadEntries(const KConfigGroup &cg);
    void clearEntries();

    int maxItems() const
    {
        return m_maxItems;
    }

    RecentItemsModel::RecentItem const *getItem(const QModelIndex &) const;
    RecentItemsModel::RecentItem const *getItem(int index) const;

    // Model implementation:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::ItemDataRole::DisplayRole) const override;

private:
    QList<RecentItemsModel::RecentItem> m_recentItems;

    int m_maxItems = 20;
    QFileIconProvider m_iconProvider;
};

#endif // RECENTITEMSMODEL_H
