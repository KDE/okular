/*
 *   Copyright 2011 Marco Martin <mart@kde.org>
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
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.extras 0.1 as PlasmaExtra
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.mobilecomponents 0.1 as MobileComponents
import org.kde.qtextracomponents 0.1
import org.kde.okular 0.1 as Okular

MouseEventListener {
    id: root
    //+1: switch to next image on mouse release
    //-1: switch to previous image on mouse release
    //0: do nothing
    property int delta


    property Item flickable: mainFlickable
    property bool pageSwitchEnabled: false
    property alias document: mainPage.document
    property alias pageNumber: mainPage.pageNumber
    property Item pageItem: mainPage

    onWheelMoved: {
        if (wheel.modifiers == Qt.ControlModifier) {
            var factor = wheel.delta > 0 ? 1.1 : 0.9
            if (scale(factor)) {
                pageArea.oldDelegate.scale(mainPage.width / mainPage.implicitWidth, true)
            }
        }
    }

    function scale(zoom, absolute) {
        var newScale = absolute ? zoom : (mainPage.width / mainPage.implicitWidth) * zoom;
        if (newScale < 0.3 || newScale > 3) {
            return false
        }

        if (imageMargin.zooming) {
            // pinch is happening!
            mainPage.width = imageMargin.startWidth * zoom
            mainPage.height = imageMargin.startHeight * zoom
        } else if (absolute) {
            // we were given an absolute, not a relative, scale
            mainPage.width = mainPage.implicitWidth * zoom
            mainPage.height = mainPage.implicitHeight * zoom
        } else {
            mainPage.width *= zoom
            mainPage.height *= zoom
        }

        return true
    }

    Rectangle {
        id: backgroundRectangle
        x: -mainFlickable.contentX + mainPage.x
        y: 0
        anchors {
            top: parent.top
            bottom: parent.bottom
        }
        width: mainPage.width
        color: "white"

        Image {
            source: "image://appbackgrounds/shadow-left"
            fillMode: Image.TileVertically
            opacity: 0.5
            anchors {
                right: parent.left
                top: parent.top
                bottom: parent.bottom
            }
        }
        Image {
            source: "image://appbackgrounds/shadow-right"
            fillMode: Image.TileVertically
            opacity: 0.5
            anchors {
                left: parent.right
                top: parent.top
                bottom: parent.bottom
            }
        }
    }

    Flickable {
        id: mainFlickable
        property real ratio : width / height
        anchors.fill: parent
        width: parent.width
        height: parent.height
        contentWidth: imageMargin.width
        contentHeight: imageMargin.height

        onContentXChanged: {
            if (atXBeginning && contentX < 0) {
                root.delta = -1
            } else if (atXEnd) {
                root.delta = +1
            } else {
                root.delta = 0
            }
        }

        PinchArea {
            id: imageMargin
            width: Math.max(mainFlickable.width + (pageSwitchEnabled ? 1: 0), mainPage.width)
            height: Math.max(mainFlickable.height, mainPage.height)

            property real startWidth
            property real startHeight
            property real startY
            property real startX
            property bool zooming: false
            onPinchStarted: {
                startWidth = mainPage.width
                startHeight = mainPage.height
                zooming = true
                startY = pinch.center.y
                startX = pinch.center.x
                pageArea.oldDelegate.visible = false
            }
            onPinchUpdated: {
                var deltaWidth = mainPage.width < imageMargin.width ? ((startWidth * pinch.scale) - mainPage.width) : 0
                var deltaHeight = mainPage.height < imageMargin.height ? ((startHeight * pinch.scale) - mainPage.height) : 0
                if (root.scale(pinch.scale)) {
                    mainFlickable.contentY += pinch.previousCenter.y - pinch.center.y + startY * (pinch.scale - pinch.previousScale) - deltaHeight
                    mainFlickable.contentX += pinch.previousCenter.x - pinch.center.x + startX * (pinch.scale - pinch.previousScale) - deltaWidth
                }
            }
            onPinchFinished: {
                mainFlickable.returnToBounds()
                pageArea.oldDelegate.scale(mainPage.width / mainPage.implicitWidth)
                pageArea.oldDelegate.visible = true
                zooming = false
            }

            Okular.PageItem {
                id: mainPage
                document: documentItem
                flickable: mainFlickable
                property real ratio: implicitWidth / implicitHeight

                x: Math.round((parent.width - width) / 2)
                y: Math.round((parent.height - height) / 2)
                width: implicitWidth
                height: implicitHeight
            }
        }
    }
    Image {
        source: "bookmark.png"
        anchors {
            top: parent.top
            right: backgroundRectangle.right
            rightMargin: -5
            topMargin: mainPage.bookmarked ? -30 : -120
        }
        Behavior on anchors.topMargin {
                NumberAnimation {
                    duration: 250
                }
            }

        MouseArea {
            anchors {
                fill: parent
                margins: -8
            }
            onClicked: mainPage.bookmarked = !mainPage.bookmarked
        }
    }
}
