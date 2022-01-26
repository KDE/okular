/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.okular 2.0 as Okular
import org.kde.kirigami 2.17 as Kirigami

Kirigami.Page {
    id: root

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

    Connections {
        target: root.document

        function onError(text, duration) {
            inlineMessage.showMessage(Kirigami.MessageType.Error, text,  duration);
        }

        function onWarning(text, duration) {
            inlineMessage.showMessage(Kirigami.MessageType.Warning, text,  duration);
        }

        function onNotice(text, duration) {
            inlineMessage.showMessage(Kirigami.MessageType.Information, text,  duration);
        }
    }

    Kirigami.InlineMessage {
        id: inlineMessage
        width: parent.width

        function showMessage(type, text, duration) {
            inlineMessage.type = type;
            inlineMessage.text = text;
            inlineMessage.visible = true;
            inlineMessageTimer.interval = duration > 0 ? duration : 500 + 100 * text.length;
        }

        onVisibleChanged: {
            if (visible) {
                inlineMessageTimer.start()
            } else {
                inlineMessageTimer.stop()
            }
        }

        Timer {
            id: inlineMessageTimer
            onTriggered: inlineMessage.visible = false
        }
    }

    Kirigami.PlaceholderMessage {
        visible: !document.opened
        text: i18n("No document open")
        helpfulAction: openDocumentAction
        width: parent.width - (Kirigami.Units.largeSpacing * 4)
        anchors.centerIn: parent
    }

    Connections {
        id: bookmarkConnection
        target: pageArea.page
        function onBookmarkedChanged() {
            actions.main.checked = pageArea.page.bookmarked
        }
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
