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
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents
import org.kde.kquickcontrolsaddons 2.0
import org.kde.okular 2.0 as Okular


MobileComponents.SplitDrawer {
    id: splitDrawer
    anchors.fill: parent
    visible: true

    property alias splitDrawerOpen: splitDrawer.open
    property alias overlayDrawerOpen: resourceBrowser.open

    //An alias doesn't work
    property bool bookmarked: false
    onBookmarkedChanged: {
        pageArea.page.bookmarked = bookmarked
    }

    drawer: Documents {
        implicitWidth: splitDrawer.width/4 * 3
    }

    MobileComponents.OverlayDrawer {
        id: resourceBrowser
        anchors.fill: parent
        visible: true

        Okular.DocumentView {
            id: pageArea
            document: documentItem
            anchors.fill: parent

            onPageChanged: {
                bookmarkConnection.target = page
                splitDrawer.bookmarked = page.bookmarked
            }
        }
        //HACK
        Connections {
            id: bookmarkConnection
            target: pageArea.page
            onBookmarkedChanged: splitDrawer.bookmarked = pageArea.page.bookmarked
        }

        drawer: Item {
            id: browserFrame
            anchors.fill: parent
            state: "Hidden"

            PlasmaComponents.ToolBar {
                id: mainToolBar

                height: units.gridUnit * 2
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
                    bottom: tabsToolbar.top
                }
                clip: true
                toolBar: mainToolBar
            }

            Connections {
                id: scrollConnection
                property int oldContentY:0
                target: pageStack.currentPage

                onContentYChanged: {
                    scrollConnection.oldContentY = pageStack.currentPage.contentY
                }
            }

            PlasmaComponents.ToolBar {
                id: tabsToolbar
                y: parent.height - tabsToolbar.height*5
                height: mainTabBar.height
                anchors {
                    top: undefined
                    bottom: browserFrame.bottom
                    left: parent.left
                    right: parent.right
                }
                tools: Item {
                    width: parent.width
                    height: childrenRect.height
                    PlasmaComponents.TabBar {
                        id: mainTabBar
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: Math.min(parent.width, implicitWidth)
                        tabPosition: Qt.BottomEdge
                        PlasmaComponents.TabButton {
                            id: thumbnailsButton
                            text: tabsToolbar.width > units.gridUnit * 30 ? i18n("Thumbnails") : ""
                            iconSource: "view-preview"
                            onCheckedChanged: {
                                if (checked) {
                                    pageStack.replace(Qt.createComponent("Thumbnails.qml"))
                                }
                            }
                        }
                        PlasmaComponents.TabButton {
                            id: tocButton
                            enabled: documentItem.tableOfContents.count > 0
                            text: tabsToolbar.width > units.gridUnit * 30 ? i18n("Table of contents") : ""
                            iconSource: "view-table-of-contents-ltr"
                            onCheckedChanged: {
                                if (checked) {
                                    pageStack.replace(Qt.createComponent("TableOfContents.qml"))
                                }
                            }
                        }
                        PlasmaComponents.TabButton {
                            id: bookmarksButton
                            enabled: documentItem.bookmarkedPages.length > 0
                            text: tabsToolbar.width > units.gridUnit * 30 ? i18n("Bookmarks") : ""
                            iconSource: "bookmarks-organize"
                            onCheckedChanged: {
                                if (checked) {
                                    pageStack.replace(Qt.createComponent("Bookmarks.qml"))
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
