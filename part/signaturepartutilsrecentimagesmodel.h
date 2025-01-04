/*
    SPDX-FileCopyrightText: 2023-2025 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef SIGNATUREPARTUTILS_RECENTIMAGESMODEL_H
#define SIGNATUREPARTUTILS_RECENTIMAGESMODEL_H

#include <QAbstractListModel>

namespace SignaturePartUtils
{

class RecentImagesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    RecentImagesModel();
    QVariant roleFromString(const QString &data, int role) const;
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    void setFileSystemSelection(const QString &selection);
    void clear();
    void removeItem(const QString &text);
    void saveBack();

private:
    std::optional<QString> m_selectedFromFileSystem;
    QStringList m_storedElements;
};
} // namespace SignaturePartUtils

#endif // SIGNATUREPARTUTILS_RECENTIMAGESMODEL_H
