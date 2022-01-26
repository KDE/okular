/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import org.kde.kirigami 2.17 as Kirigami


ThumbnailsBase {
    id: root
    header: Kirigami.AbstractApplicationHeader {
        topPadding: Kirigami.Units.largeSpacing
        bottomPadding: Kirigami.Units.largeSpacing
        rightPadding: Kirigami.Units.largeSpacing
        leftPadding: Kirigami.Units.largeSpacing
        Kirigami.Heading {
            level: 2
            text: i18n("Bookmarks")
            width: parent.width
        }
    }
    model: documentItem.bookmarkedPages
    onPageClicked: {
        pageArea.delegate.pageItem.goToBookmark(pageArea.delegate.pageItem.bookmarks[0])
    }
}
