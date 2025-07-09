/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.okular as Okular
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: root

    property alias document: pageArea.document
    property alias page: pageArea.page
    leftPadding: 0
    topPadding: 0
    rightPadding: 0
    bottomPadding: 0

    actions: Kirigami.Action {
        icon.name: pageArea.page.bookmarked ? "bookmark-remove" : "bookmarks-organize"
        checkable: true
        visible: document.opened
        onCheckedChanged: (checked) => pageArea.page.bookmarked = checked
        text: pageArea.page.bookmarked ? i18n("Remove bookmark") : i18n("Bookmark this page")
        checked: pageArea.page.bookmarked
    }

    Okular.DocumentView {
        id: pageArea
        anchors.fill: parent

        onClicked: fileBrowserRoot.controlsVisible = !fileBrowserRoot.controlsVisible
        onUrlOpened: welcomeView.saveRecentDocument(document.url)
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
        position: Kirigami.InlineMessage.Header

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

    WelcomeView {
        id: welcomeView
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
        value: documentItem.pageCount !== 0 ? ((documentItem.currentPage + 1) / documentItem.pageCount) : 0
    }
}
