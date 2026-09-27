/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.okular as Okular
import QtQuick.Layouts


Kirigami.OverlayDrawer {
    id: root

    bottomPadding: parent.SafeArea.margins.bottom
    topPadding: parent.SafeArea.margins.top
    leftPadding: root.edge === Qt.LeftEdge ? parent.SafeArea.margins.left : 0
    rightPadding: root.edge === Qt.RightEdge ? parent.SafeArea.margins.right : 0

    edge: Qt.application.layoutDirection == Qt.RightToLeft ? Qt.LeftEdge : Qt.RightEdge
    contentItem: ColumnLayout {
        id: browserFrame
        spacing: 0

        QQC2.StackView {
            id: pageStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
        }

        Connections {
            target: documentItem
            function onUrlChanged() {
                thumbnailsButton.checked = true;
            }
        }

        QQC2.ToolBar {
            id: tabsToolbar
            position: QQC2.ToolBar.Footer
            Layout.fillWidth: true
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
                    QQC2.ToolButton {
                        id: signatyresButton
                        enabled: documentItem.signaturesModel.count > 0
                        text: tabsToolbar.width > Kirigami.Units.gridUnit * 30 ? i18n("Signatures") : ""
                        icon.name: "application-pkcs7-signature"
                        checkable: true
                        flat: false
                        onCheckedChanged: {
                            if (checked) {
                                pageStack.replace(signaturesComponent)
                            }
                        }
                        QQC2.ButtonGroup.group: tabPositionGroup
                    }
                }
            }
        }
    }

    Component {
        id: signaturesComponent
        Signatures {
            onDialogOpened: {
                // We don't want to have two modal things open at the same time
                if (root.modal) {
                    root.close();
                }
            }
        }
    }
}
