/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.1
import QtQuick.Controls 2.5 as QQC2
import org.kde.kirigami 2.0 as Kirigami
import org.kde.okular 2.0 as Okular


Kirigami.OverlayDrawer {
    bottomPadding: 0
    topPadding: 0
    leftPadding: 0
    rightPadding: 0

    edge: Qt.application.layoutDirection == Qt.RightToLeft ? Qt.LeftEdge : Qt.RightEdge
    contentItem: Item {
        id: browserFrame
        implicitWidth: Kirigami.Units.gridUnit * 45
        implicitHeight: implicitWidth
        state: "Hidden"

        QQC2.StackView {
            id: pageStack
            anchors {
                left: parent.left
                top: parent.top
                right: parent.right
                bottom: tabsToolbar.top
            }
            clip: true
        }

        Connections {
            target: documentItem
            onUrlChanged: thumbnailsButton.checked = true;
        }

        QQC2.ToolBar {
            id: tabsToolbar
            height: mainTabBar.height
            position: QQC2.ToolBar.Footer
            anchors {
                top: undefined
                bottom: browserFrame.bottom
                left: parent.left
                right: parent.right
            }
            Component.onCompleted: thumbnailsButton.checked = true;
            Item {
                width: parent.width
                height: childrenRect.height
                Row {
                    id: mainTabBar
                    spacing: 0
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Math.min(parent.width, implicitWidth)
                    QQC2.ButtonGroup { id: tabPositionGroup }
                    QQC2.ToolButton {
                        id: thumbnailsButton
                        text: tabsToolbar.width > Kirigami.Units.gridUnit * 30 ? i18n("Thumbnails") : ""
                        icon.name: "view-preview"
                        checkable: true
                        flat: false
                        onCheckedChanged: {
                            if (checked) {
                                pageStack.replace(Qt.createComponent("Thumbnails.qml"))
                            }
                        }
                        QQC2.ButtonGroup.group: tabPositionGroup
                    }
                    QQC2.ToolButton {
                        id: tocButton
                        enabled: documentItem.tableOfContents.count > 0
                        text: tabsToolbar.width > Kirigami.Units.gridUnit * 30 ? i18n("Table of contents") : ""
                        icon.name: "view-table-of-contents-ltr"
                        checkable: true
                        flat: false
                        onCheckedChanged: {
                            if (checked) {
                                pageStack.replace(Qt.createComponent("TableOfContents.qml"))
                            }
                        }
                        QQC2.ButtonGroup.group: tabPositionGroup
                    }
                    QQC2.ToolButton {
                        id: bookmarksButton
                        enabled: documentItem.bookmarkedPages.length > 0
                        text: tabsToolbar.width > Kirigami.Units.gridUnit * 30 ? i18n("Bookmarks") : ""
                        icon.name: "bookmarks-organize"
                        checkable: true
                        flat: false
                        onCheckedChanged: {
                            if (checked) {
                                pageStack.replace(Qt.createComponent("Bookmarks.qml"))
                            }
                        }
                        QQC2.ButtonGroup.group: tabPositionGroup
                    }
                }
            }
        }
    }
}
