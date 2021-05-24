/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.1


ThumbnailsBase {
    model: documentItem.bookmarkedPages
    onPageClicked: {
        pageArea.delegate.pageItem.goToBookmark(pageArea.delegate.pageItem.bookmarks[0])
    }
}
