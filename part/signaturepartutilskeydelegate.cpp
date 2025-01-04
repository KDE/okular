/*
    SPDX-FileCopyrightText: 2023-2025 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "signaturepartutilskeydelegate.h"

#include "signaturepartutilsmodel.h"

#include <QApplication>
#include <QPainter>
#include <QPen>

namespace SignaturePartUtils
{

QSize KeyDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto baseSize = QStyledItemDelegate::sizeHint(option, index);
    baseSize.setHeight(baseSize.height() * 2);
    return baseSize;
}

void KeyDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto style = option.widget ? option.widget->style() : QApplication::style();

    // This paints the background without initializing the
    // styleoption from the actual index. Given we want default background
    // and paint the foreground a bit later
    // This accomplishes it quite nicely.
    style->drawControl(QStyle::CE_ItemViewItem, &option, painter, option.widget);

    QPalette::ColorGroup cg;
    if (option.state & QStyle::State_Active) {
        cg = QPalette::Normal;
    } else {
        cg = QPalette::Inactive;
    }

    if (option.state & QStyle::State_Selected) {
        painter->setPen(QPen {option.palette.brush(cg, QPalette::HighlightedText), 0});
    } else {
        painter->setPen(QPen {option.palette.brush(cg, QPalette::Text), 0});
    }

    auto textRect = option.rect;
    int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &option, option.widget) + 1;
    if (showIcon) {
        textRect.adjust(textRect.height() + textMargin, 0, 0, 0); // make space for icon
    }
    textRect.adjust(textMargin, 0, -textMargin, 0);

    QRect topHalf {textRect.x(), textRect.y(), textRect.width(), textRect.height() / 2};
    QRect bottomHalf {textRect.x(), textRect.y() + textRect.height() / 2, textRect.width(), textRect.height() / 2};

    style->drawItemText(painter, topHalf, (option.displayAlignment & Qt::AlignVertical_Mask) | Qt::AlignLeft, option.palette, true, index.data(NickDisplayRole).toString());
    style->drawItemText(painter, bottomHalf, (option.displayAlignment & Qt::AlignVertical_Mask) | Qt::AlignRight, option.palette, true, index.data(EmailRole).toString());
    style->drawItemText(painter, bottomHalf, (option.displayAlignment & Qt::AlignVertical_Mask) | Qt::AlignLeft, option.palette, true, index.data(CommonNameRole).toString());
    if (showIcon) {
        if (auto icon = index.data(Qt::DecorationRole).value<QIcon>(); !icon.isNull()) {
            icon.paint(painter, QRect(option.rect.topLeft(), QSize(textRect.height(), textRect.height())));
        }
    }
}

} // namespace SignaturePartUtils
