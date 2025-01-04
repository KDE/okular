/*
    SPDX-FileCopyrightText: 2023-2025 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef SIGNATUREPARTUTILS_KEYDELEGATE_H
#define SIGNATUREPARTUTILS_KEYDELEGATE_H

#include <QStyledItemDelegate>

namespace SignaturePartUtils
{
class KeyDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const final;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const final;
    bool showIcon = false;
};

} // namespace SignaturePartUtils

#endif // SIGNATUREPARTUTILS_KEYDELEGATE_H
