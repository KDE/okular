/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.1
import QtQuick.Controls 2.3 as QQC2
import org.kde.okular 2.0 as Okular
import org.kde.kirigami 2.10 as Kirigami

Kirigami.Page {
    property alias document: pageArea.document
    leftPadding: 0
    topPadding: 0
    rightPadding: 0
    bottomPadding: 0

    actions.main: Kirigami.Action {
        icon.name: pageArea.page.bookmarked ? "bookmark-remove" : "bookmarks-organize"
        checkable: true
        onCheckedChanged: pageArea.page.bookmarked = checked
        text: pageArea.page.bookmarked ? i18n("Remove bookmark") : i18n("Bookmark this page")
    }

    Okular.DocumentView {
        id: pageArea
        anchors.fill: parent

        onPageChanged: {
            bookmarkConnection.target = page
            actions.main.checked = page.bookmarked
        }
        onClicked: fileBrowserRoot.controlsVisible = !fileBrowserRoot.controlsVisible
    }

    // TODO KF 5.64 replace usage by upstream PlaceholderMessage
    PlaceholderMessage {
        visible: documentItem.url.toString().length === 0
        text: i18n("No document open")
        helpfulAction: openDocumentAction
        width: parent.width - (Kirigami.Units.largeSpacing * 4)
        anchors.centerIn: parent
    }

    Connections {
        id: bookmarkConnection
        target: pageArea.page
        onBookmarkedChanged: actions.main.checked = pageArea.page.bookmarked
    }
    QQC2.ProgressBar {
        id: bar
        z: 99
        visible: applicationWindow().controlsVisible
        height: Kirigami.Units.smallSpacing
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        value: documentItem.pageCount !== 0 ? ((documentItem.currentPage+1) / documentItem.pageCount) : 0
    }
}
