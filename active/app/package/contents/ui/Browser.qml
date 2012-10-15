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

import QtQuick 1.1
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.extras 0.1 as PlasmaExtras
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.mobilecomponents 0.1 as MobileComponents
import org.kde.qtextracomponents 0.1
import org.kde.okular 0.1 as Okular


MobileComponents.OverlayDrawer {
    id: resourceBrowser
    property string currentUdi
    anchors.fill: parent

    MouseEventListener {
        id: pageArea
        anchors.fill: parent
        //enabled: !delegate.interactive
        property Item delegate: delegate1
        property Item oldDelegate: delegate2
        property bool incrementing: delegate.delta > 0
        Connections {
            target: pageArea.delegate
            onDeltaChanged: {
                pageArea.oldDelegate.delta = pageArea.delegate.delta
                if (pageArea.delegate.delta > 0) {
                    pageArea.oldDelegate.visible = true
                    pageArea.oldDelegate.pageNumber = pageArea.delegate.pageNumber + 1
                    documentItem.currentPage = pageArea.oldDelegate.pageNumber
                    pageArea.oldDelegate.visible = !(pageArea.delegate.pageNumber == documentItem.pageCount-1)
                } else if (pageArea.delegate.delta < 0) {
                    pageArea.oldDelegate.pageNumber =  pageArea.delegate.pageNumber - 1
                    documentItem.currentPage = pageArea.oldDelegate.pageNumber

                    pageArea.oldDelegate.visible = pageArea.delegate.pageNumber != 0
                }
            }
        }

        property int startMouseScreenX
        property int startMouseScreenY
        onPressed: {
            startMouseScreenX = mouse.screenX
            startMouseScreenY = mouse.screenY
        }
        onPositionChanged: {
            if (Math.abs(mouse.screenX - startMouseScreenX) > width/5) {
                delegate.pageSwitchEnabled = true
            }
        }
        onReleased: {
            delegate.pageSwitchEnabled = false
            if (Math.abs(mouse.screenX - startMouseScreenX) < 20 &&
                Math.abs(mouse.screenY - startMouseScreenY) < 20) {
                if (browserFrame.state == "Closed") {
                    browserFrame.state = "Hidden"
                } else {
                    browserFrame.state = "Closed"
                }

            } else if (oldDelegate.visible && delegate.delta != 0 &&
                (Math.abs(mouse.screenX - startMouseScreenX) > width/5) &&
                 Math.abs(mouse.screenX - startMouseScreenX) > Math.abs(mouse.screenY - startMouseScreenY)) {
                oldDelegate = delegate
                delegate = (delegate == delegate1) ? delegate2 : delegate1
                switchAnimation.running = true
            }
        }
        FullScreenDelegate {
            id: delegate2
            width: parent.width
            height: parent.height
        }
        FullScreenDelegate {
            id: delegate1
            width: parent.width
            height: parent.height
            Component.onCompleted: pageNumber = documentItem.currentPage
        }

        SequentialAnimation {
            id: switchAnimation
            NumberAnimation {
                target: pageArea.oldDelegate
                properties: "x"
                to: pageArea.incrementing ? -pageArea.oldDelegate.width : pageArea.oldDelegate.width
                easing.type: Easing.InQuad
                duration: 250
            }
            ScriptAction {
                script: {
                    pageArea.oldDelegate.z = 0
                    pageArea.delegate.z = 10
                    pageArea.oldDelegate.x = 0
                    pageArea.delegate.x = 0
                }
            }
            ScriptAction {
                script: delegate1.delta = delegate2.delta = 0
            }
        }
    }
    PlasmaComponents.ScrollBar {
        flickableItem: pageArea.delegate.flickable
        orientation: Qt.Vertical
        anchors {
            right: pageArea.right
            top: pageArea.top
            bottom: pageArea.bottom
            left: undefined
        }
    }
    PlasmaComponents.ScrollBar {
        flickableItem: pageArea.delegate.flickable
        orientation: Qt.Horizontal
        anchors {
            left: pageArea.left
            right: pageArea.right
            bottom: pageArea.bottom
            top: undefined
        }
    }

    drawer: Item {
        id: browserFrame
        anchors.fill: parent
        state: "Hidden"


        PlasmaComponents.ToolBar {
            id: mainToolBar

            y: pageStack.currentPage.contentY <= 0 ? 0 : -height
            transform: Translate {
                y: Math.max(0, -pageStack.currentPage.contentY)
            }
            tools: pageStack.currentPage.tools
            Behavior on y {
                NumberAnimation {
                    duration: 250
                }
            }
            anchors {
                left: parent.left
                right: parent.right
            }
        }


        PlasmaComponents.PageStack {
            id: pageStack
            anchors {
                left: parent.left
                top: mainToolBar.bottom
                right: parent.right
                bottom: parent.bottom
            }
            clip: true
            toolBar: mainToolBar
        }

        Connections {
            id: scrollConnection
            property int oldContentY:0
            target: pageStack.currentPage

            onContentYChanged: {

                if (pageStack.currentPage.contentHeight <= pageStack.height ||
                    (scrollConnection.oldContentY < pageStack.currentPage.contentY &&
                     pageStack.currentPage.contentY > 0)) {
                    tabsToolbar.y = tabsToolbar.parent.height - tabsToolbar.height
                } else {
                    tabsToolbar.y = tabsToolbar.parent.height
                }
                scrollConnection.oldContentY = pageStack.currentPage.contentY
            }
        }

        PlasmaComponents.ToolBar {
            id: tabsToolbar
            y: parent.height
            anchors {
                top: undefined
                bottom: undefined
                left: parent.left
                right: parent.right
            }
            tools: Item {
                width: parent.width
                height: childrenRect.height
                PlasmaComponents.TabBar {
                    id: mainTabBar
                    anchors.horizontalCenter: parent.horizontalCenter
                    PlasmaComponents.TabButton {
                        id: thumbnailsButton
                        text: i18n("Thumbnails")
                        onCheckedChanged: {
                            if (checked) {
                                pageStack.replace(Qt.createComponent("Thumbnails.qml"))
                            }
                        }
                    }
                    PlasmaComponents.TabButton {
                        id: tocButton
                        enabled: documentItem.tableOfContents.count > 0
                        text: i18n("Table of contents")
                        onCheckedChanged: {
                            if (checked) {
                                pageStack.replace(Qt.createComponent("TableOfContents.qml"))
                            }
                        }
                    }
                    PlasmaComponents.TabButton {
                        id: bookmarksButton
                        enabled: documentItem.bookmarkedPages.length > 0
                        text: i18n("Bookmarks")
                        onCheckedChanged: {
                            if (checked) {
                                pageStack.replace(Qt.createComponent("Bookmarks.qml"))
                            }
                        }
                    }
                }
            }
            Behavior on y {
                NumberAnimation {
                    duration: 250
                }
            }
        }
    }
}

