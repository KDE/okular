/*
    SPDX-FileCopyrightText: 2023-2025 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef SIGNATUREPARTUTILS_IMAGEITEMDELEGATE_H
#define SIGNATUREPARTUTILS_IMAGEITEMDELEGATE_H

#include <QStyledItemDelegate>

namespace SignaturePartUtils
{

class ImageItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

} // namespace SignaturePartUtils

#endif // SIGNATUREPARTUTILS_IMAGEITEMDELEGATE_H
