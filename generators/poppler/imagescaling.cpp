/*
    SPDX-FileCopyrightText: 2023 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "imagescaling.h"
#include <QPainter>

QImage imagescaling::scaleAndFitCanvas(const QImage &input, QSize expectedSize)
{
    if (input.size() == expectedSize) {
        return input;
    }
    auto scaled = input.scaled(expectedSize, Qt::KeepAspectRatio);
    if (scaled.size() == expectedSize) {
        return scaled;
    }
    QImage canvas(expectedSize, QImage::Format_ARGB32);
    canvas.fill(Qt::transparent);
    auto scaledSize = scaled.size();
    QPoint topLeft((expectedSize.width() - scaledSize.width()) / 2, (expectedSize.height() - scaledSize.height()) / 2);
    QPainter painter(&canvas);
    painter.drawImage(topLeft, scaled);
    return canvas;
}
