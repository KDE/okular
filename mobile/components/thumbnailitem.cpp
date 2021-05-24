/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "thumbnailitem.h"

ThumbnailItem::ThumbnailItem(QQuickItem *parent)
    : PageItem(parent)
{
    setIsThumbnail(true);
}

ThumbnailItem::~ThumbnailItem()
{
}
