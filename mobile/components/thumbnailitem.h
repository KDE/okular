/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef THUMBNAILITEM_H
#define THUMBNAILITEM_H

#include "pageitem.h"

class ThumbnailItem : public PageItem
{
    Q_OBJECT

public:
    explicit ThumbnailItem(QQuickItem *parent = nullptr);
    ~ThumbnailItem() override;
};

#endif
