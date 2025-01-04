/*
    SPDX-FileCopyrightText: 2023-2025 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "signaturepartutilsrecentimagesmodel.h"
#include "signaturepartutilskconfig.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QFile>

namespace SignaturePartUtils
{
RecentImagesModel::RecentImagesModel()
{
    const auto recentList = KSharedConfig::openConfig()->group(ConfigGroup()).readEntry<QStringList>(ConfigBackgroundKey(), QStringList());
    for (const auto &element : recentList) {
        if (QFile::exists(element)) { // maybe the image has been removed from disk since last invocation
            m_storedElements.push_back(element);
        }
    }
}

QVariant RecentImagesModel::roleFromString(const QString &data, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return data;
    default:
        return QVariant();
    }
}

QVariant RecentImagesModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid));
    int row = index.row();
    if (m_selectedFromFileSystem.has_value()) {
        if (row == 0) {
            return roleFromString(*m_selectedFromFileSystem, role);
        } else {
            row--;
        }
    }
    if (row < m_storedElements.size()) {
        return roleFromString(m_storedElements.at(row), role);
    }
    return QVariant();
}

int RecentImagesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_storedElements.size() + (m_selectedFromFileSystem.has_value() ? 1 : 0);
}

void RecentImagesModel::setFileSystemSelection(const QString &selection)
{
    if (m_storedElements.contains(selection)) {
        return;
    }
    if (selection.isEmpty()) {
        if (!m_selectedFromFileSystem) {
            return;
        }
        beginRemoveRows(QModelIndex(), 0, 0);
        m_selectedFromFileSystem.reset();
        endRemoveRows();
        return;
    }
    if (!QFile::exists(selection)) {
        return;
    }
    if (m_selectedFromFileSystem) {
        m_selectedFromFileSystem = selection;
        Q_EMIT dataChanged(index(0, 0), index(0, 0));
    } else {
        beginInsertRows(QModelIndex(), 0, 0);
        m_selectedFromFileSystem = selection;
        endInsertRows();
    }
}

void RecentImagesModel::clear()
{
    beginResetModel();
    m_selectedFromFileSystem = {};
    m_storedElements.clear();
    endResetModel();
}

void RecentImagesModel::removeItem(const QString &text)
{
    if (text == m_selectedFromFileSystem) {
        beginRemoveRows(QModelIndex(), 0, 0);
        m_selectedFromFileSystem.reset();
        endRemoveRows();
        return;
    }
    auto elementIndex = m_storedElements.indexOf(text);
    auto beginRemove = elementIndex;
    if (m_selectedFromFileSystem) {
        beginRemove++;
    }
    beginRemoveRows(QModelIndex(), beginRemove, beginRemove);
    m_storedElements.removeAt(elementIndex);
    endRemoveRows();
}

void RecentImagesModel::saveBack()
{
    QStringList elementsToStore = m_storedElements;
    if (m_selectedFromFileSystem) {
        elementsToStore.push_front(*m_selectedFromFileSystem);
    }
    while (elementsToStore.size() > 3) {
        elementsToStore.pop_back();
    }
    KSharedConfig::openConfig()->group(ConfigGroup()).writeEntry(ConfigBackgroundKey(), elementsToStore);
}

} // namespace SignaturePartUtils
