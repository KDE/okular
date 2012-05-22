/*
 *   Copyright 2011 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
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


PlasmaComponents.Page {
    id: resourceBrowser
    state: "toolsClosed"
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
                    resultsGrid.currentIndex = pageArea.oldDelegate.pageNumber
                    documentItem.currentPage = pageArea.oldDelegate.pageNumber

                    pageArea.oldDelegate.visible = !(pageArea.delegate.pageNumber == documentItem.pageCount-1)
                } else if (pageArea.delegate.delta < 0) {
                    pageArea.oldDelegate.pageNumber =  pageArea.delegate.pageNumber - 1
                    resultsGrid.currentIndex = pageArea.oldDelegate.pageNumber
                    documentItem.currentPage = pageArea.oldDelegate.pageNumber

                    pageArea.oldDelegate.visible = pageArea.delegate.pageNumber != 0
                }
            }
        }

        property int startX
        property int starty
        onPressed: {
            startX = mouse.screenX
            startY = mouse.screenY
        }
        onPositionChanged: {
            if (Math.abs(mouse.screenX - startX) > width/5) {
                delegate.pageSwitchEnabled = true
            }
        }
        onReleased: {
            delegate.pageSwitchEnabled = false
            if (Math.abs(mouse.screenX - startX) < 20 &&
                Math.abs(mouse.screenY - startY) < 20) {
                if (resourceBrowser.state == "toolsOpen") {
                    resourceBrowser.state = "toolsClosed"
                } else {
                    resourceBrowser.state = "toolsOpen"
                }
            } else if (oldDelegate.visible && delegate.delta != 0 &&
                delegate.doSwitch &&
                ((incrementing && mouse.x <= width/5) ||
                  !incrementing && mouse.x >= (width/5)*4)) {
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
        }
    }
    PlasmaComponents.ScrollBar {
        flickableItem: pageArea.delegate.flickable
        orientation: Qt.Horizontal
        anchors {
            left: pageArea.left
            right: pageArea.right
            bottom: pageArea.bottom
        }
    }

    Image {
        id: browserFrame
        z: 100
        source: "image://appbackgrounds/standard"
        fillMode: Image.Tile
        anchors {
            top: parent.top
            bottom: parent.bottom
        }
        width: parent.width - handleGraphics.width
        x: parent.width + extraSpace
        property bool open: false
        property int extraSpace: resourceBrowser.state == "toolsOpen" ? 0 : handleGraphics.width
        Behavior on extraSpace {
            NumberAnimation {
                duration: 250
                easing.type: Easing.InOutQuad
            }
        }


        Image {
            source: "image://appbackgrounds/shadow-left"
            fillMode: Image.TileVertically
            anchors {
                right: parent.left
                top: parent.top
                bottom: parent.bottom
                rightMargin: -1
            }
        }
        PlasmaCore.FrameSvgItem {
            id: handleGraphics
            imagePath: "dialogs/background"
            enabledBorders: "LeftBorder|TopBorder|BottomBorder"
            width: handleIcon.width + margins.left + margins.right + 4
            height: handleIcon.width * 1.6 + margins.top + margins.bottom + 4
            anchors {
                right: parent.left
                verticalCenter: parent.verticalCenter
            }

            PlasmaCore.SvgItem {
                id: handleIcon
                svg: PlasmaCore.Svg {imagePath: "toolbar-icons/show"}
                elementId: "show-menu"
                x: parent.margins.left
                y: parent.margins.top
                width: theme.smallMediumIconSize
                height: width
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        MouseEventListener {
            anchors {
                top: parent.top
                bottom: parent.bottom
                left: handleGraphics.left
                right: parent.right
            }

            property int startX
            property real oldMouseScreenX
            property bool toggle: true
            property bool startDragging: false
            onPressed: {
                startX = browserFrame.x
                oldMouseScreenX = mouse.screenX
                startMouseScreenX = mouse.screenX
                toggle = true
                startDragging = false
            }
            onPositionChanged: {
                //mouse over handle and didn't move much
                if (mouse.x > handleGraphics.width ||
                    Math.abs(mouse.screenX - startMouseScreenX) > 20) {
                    toggle = false
                }

                if (mouse.x < handleGraphics.width ||
                    Math.abs(mouse.screenX - startMouseScreenX) > resourceBrowser.width / 5) {
                    startDragging = true
                }
                if (startDragging) {
                    browserFrame.x = Math.max(resourceBrowser.width - browserFrame.width, browserFrame.x + mouse.screenX - oldMouseScreenX)
                }
                oldMouseScreenX = mouse.screenX
            }
            onReleased: {
                if (toggle) {
                    browserFrame.open = !browserFrame.open
                } else {
                    browserFrame.open = Math.abs(browserFrame.x - startX) > resourceBrowser.width / 3 ? !browserFrame.open : browserFrame.open
                }
                browserFrameSlideAnimation.to = browserFrame.open ? handleGraphics.width : resourceBrowser.width + browserFrame.extraSpace
                browserFrameSlideAnimation.running = true
            }

            PlasmaExtras.ScrollArea {
                anchors {
                    fill: parent
                    leftMargin: handleGraphics.width
                }
                GridView {
                    id: resultsGrid
                    anchors.fill: parent
                    clip: true

                    model: documentItem.matchingPages
                    cellWidth: theme.defaultFont.mSize.width * 14
                    cellHeight: theme.defaultFont.mSize.height * 12
                    currentIndex: documentItem.currentPage

                    delegate: Item {
                        width: resultsGrid.cellWidth
                        height: resultsGrid.cellHeight
                        PlasmaCore.FrameSvgItem {
                            anchors.centerIn: parent
                            imagePath: "widgets/media-delegate"
                            prefix: "picture"
                            width: thumbnail.width + margins.left + margins.right
                            //FIXME: why bindings with thumbnail.height doesn't work?
                            height: thumbnail.height + margins.top + margins.bottom
                            Okular.ThumbnailItem {
                                id: thumbnail
                                x: parent.margins.left
                                y: parent.margins.top
                                document: documentItem
                                pageNumber: modelData
                                width: theme.defaultFont.mSize.width * 10
                                height: Math.round(width / (implicitWidth/implicitHeight))
                                Rectangle {
                                    width: childrenRect.width
                                    height: childrenRect.height
                                    color: theme.backgroundColor
                                    radius: width
                                    smooth: true
                                    anchors {
                                        bottom: parent.bottom
                                        right: parent.right
                                    }
                                    PlasmaComponents.Label {
                                        text: modelData + 1
                                    }
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    resultsGrid.currentIndex = index
                                    pageArea.delegate.pageNumber = modelData
                                    documentItem.currentPage = modelData
                                    browserFrame.open = false

                                    browserFrameSlideAnimation.to =  resourceBrowser.width
                                    browserFrameSlideAnimation.running = true
                                }
                            }
                        }
                    }
                    highlight: PlasmaComponents.Highlight {}
                    header: PlasmaComponents.ToolBar {
                        width: resultsGrid.width
                        height: searchField.height + 10
                        MobileComponents.ViewSearch {
                            id: searchField
                            enabled: documentItem.supportsSearch
                            anchors.centerIn: parent
                            busy: documentItem.searchInProgress
                            onSearchQueryChanged: {
                                print(searchQuery)
                                if (searchQuery.length > 2) {
                                    documentItem.searchText(searchQuery)
                                } else {
                                    documentItem.resetSearch()
                                }
                            }
                        }
                        PlasmaComponents.Label {
                            anchors {
                                left: searchField.right
                                verticalCenter: searchField.verticalCenter
                            }
                            visible: documentItem.matchingPages.length == 0
                            text: i18n("No results found.")
                        }
                    }
                }
            }
        }

        //FIXME: use a state machine
        SequentialAnimation {
            id: browserFrameSlideAnimation
            property alias to: actualSlideAnimation.to
            NumberAnimation {
                id: actualSlideAnimation
                target: browserFrame
                properties: "x"
                duration: 250
                easing.type: Easing.InOutQuad
            }
        }
    }
}

