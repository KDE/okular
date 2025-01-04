/*
    SPDX-FileCopyrightText: 2023-2025 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "signaturepartutilsimageitemdelegate.h"

#include <QApplication>
#include <QImageReader>
#include <QListView>
#include <QPainter>

namespace SignaturePartUtils
{

static QImage scaleAndFitCanvas(const QImage &input, const QSize expectedSize)
{
    if (input.size() == expectedSize) {
        return input;
    }
    const auto scaled = input.scaled(expectedSize, Qt::KeepAspectRatio);
    if (scaled.size() == expectedSize) {
        return scaled;
    }
    QImage canvas(expectedSize, QImage::Format_ARGB32);
    canvas.fill(Qt::transparent);
    const auto scaledSize = scaled.size();
    QPoint topLeft((expectedSize.width() - scaledSize.width()) / 2, (expectedSize.height() - scaledSize.height()) / 2);
    QPainter painter(&canvas);
    painter.drawImage(topLeft, scaled);
    return canvas;
}

void ImageItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto style = option.widget ? option.widget->style() : QApplication::style();
    // This paints the background without initializing the
    // styleoption from the actual index. Given we want default background
    // and paint the foreground a bit later
    // This accomplishes it quite nicely.
    style->drawControl(QStyle::CE_ItemViewItem, &option, painter, option.widget);
    const auto path = index.data(Qt::DisplayRole).value<QString>();

    QImageReader reader(path);
    const QSize imageSize = reader.size();
    if (!reader.size().isNull()) {
        reader.setScaledSize(imageSize.scaled(option.rect.size(), Qt::KeepAspectRatio));
    }
    const auto input = reader.read();
    if (!input.isNull()) {
        const auto scaled = scaleAndFitCanvas(input, option.rect.size());
        painter->drawImage(option.rect.topLeft(), scaled);
    }
}

QSize ImageItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    QSize defaultSize(10, 10); // let's start with a square
    if (auto view = qobject_cast<QListView *>(option.styleObject)) {
        auto frameRect = view->frameRect().size();
        frameRect.setWidth((frameRect.width() - view->style()->pixelMetric(QStyle::PM_ScrollBarExtent)) / 2 - 2 * view->frameWidth() - view->spacing());
        return defaultSize.scaled(frameRect, Qt::KeepAspectRatio);
    }
    return defaultSize;
}

} // namespace SignaturePartUtils
