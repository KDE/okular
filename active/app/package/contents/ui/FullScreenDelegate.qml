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
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.extras 0.1 as PlasmaExtras
import org.kde.plasma.mobilecomponents 0.1 as MobileComponents
import org.kde.qtextracomponents 0.1
import org.kde.okular 0.1 as Okular

Item {
    id: root
    //+1: switch to next image on mouse release
    //-1: switch to previous image on mouse release
    //0: do nothing
    property int delta
    //if true when released will switch the delegate
    property bool doSwitch: false

    property alias document: mainPage.document
    property alias pageNumber: mainPage.pageNumber
    property string label: model["label"]

    function scale(zoom)
    {
        mainPage.width = mainPage.implicitWidth * zoom
        mainPage.height = mainPage.implicitHeight * zoom
    }
    Rectangle {
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
                root.doSwitch = (contentX < -mainFlickable.width/4)
            } else if (atXEnd) {
                root.delta = +1
                root.doSwitch = (contentX + mainFlickable.width - contentWidth > mainFlickable.width/4)
            } else {
                root.delta = 0
                root.doSwitch = false
            }
        }

        PinchArea {
            id: imageMargin
            width: Math.max(mainFlickable.width+1, mainPage.width)
            height: Math.max(mainFlickable.height, mainPage.height)

            property real startWidth
            property real startHeight
            property real startY
            property real startX
            onPinchStarted: {
                startWidth = mainPage.width
                startHeight = mainPage.height
                startY = pinch.center.y
                startX = pinch.center.x
            }
            onPinchUpdated: {
                var deltaWidth = mainPage.width < imageMargin.width ? ((startWidth * pinch.scale) - mainPage.width) : 0
                var deltaHeight = mainPage.height < imageMargin.height ? ((startHeight * pinch.scale) - mainPage.height) : 0
                mainPage.width = startWidth * pinch.scale
                mainPage.height = startHeight * pinch.scale

                mainFlickable.contentY += pinch.previousCenter.y - pinch.center.y + startY * (pinch.scale - pinch.previousScale) - deltaHeight

                mainFlickable.contentX += pinch.previousCenter.x - pinch.center.x + startX * (pinch.scale - pinch.previousScale) - deltaWidth
                pageArea.oldDelegate.scale(mainPage.width / mainPage.implicitWidth)
            }

            Okular.PageItem {
                id: mainPage
                document: documentItem
                anchors.centerIn: parent
                property real ratio: implicitWidth / implicitHeight

                width: implicitWidth
                height: implicitHeight
            }
        }
    }
}
