/*
 *   Copyright 2012 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2,
 *   or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.1
import QtQuick.Controls 2.5 as QQC2
import org.kde.kirigami 2.0 as Kirigami
import org.kde.okular 2.0 as Okular


Kirigami.OverlayDrawer {
    edge: Qt.RightEdge
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
