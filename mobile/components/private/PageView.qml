/*
 *   Copyright 2015 by Marco Martin <mart@kde.org>
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

import QtQuick 2.2
import QtGraphicalEffects 1.0
import org.kde.okular 2.0
import org.kde.kirigami 2.2 as Kirigami

Item {
    width: parent.width
    height: parent.height
    readonly property PageItem pageItem: page
    property alias document: page.document
    property alias pageNumber: page.pageNumber
    implicitWidth: page.implicitWidth
    implicitHeight: page.implicitHeight
    readonly property real pageRatio: page.implicitWidth / page.implicitHeight
    readonly property real scaleFactor: page.width / page.implicitWidth

    PageItem {
        id: page
        property bool sameOrientation: parent.width / parent.height > pageRatio
        anchors.centerIn: parent
        width: sameOrientation ? parent.height * pageRatio : parent.width
        height: !sameOrientation ? parent.width / pageRatio : parent.height
        document: null
    }

    Rectangle {
        id: backgroundRectangle
        anchors {
            top: parent.top
            bottom: parent.bottom
            left: page.left
            right: page.right
            topMargin: -Kirigami.Units.gridUnit
            bottomMargin: -Kirigami.Units.gridUnit
        }
        z: -1
        color: "white"

        LinearGradient {
            width: Kirigami.Units.gridUnit
            anchors {
                right: parent.left
                top: parent.top
                bottom: parent.bottom
            }
            start: Qt.point(0, 0)
            end: Qt.point(Kirigami.Units.gridUnit, 0)
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: "transparent"
                }
                GradientStop {
                    position: 0.7
                    color: Qt.rgba(0, 0, 0, 0.08)
                }
                GradientStop {
                    position: 1.0
                    color: Qt.rgba(0, 0, 0, 0.2)
                }
            }
        }

        LinearGradient {
            width: Kirigami.Units.gridUnit
            anchors {
                left: parent.right
                top: parent.top
                bottom: parent.bottom
            }
            start: Qt.point(0, 0)
            end: Qt.point(Kirigami.Units.gridUnit, 0)
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Qt.rgba(0, 0, 0, 0.2)
                }
                GradientStop {
                    position: 0.3
                    color: Qt.rgba(0, 0, 0, 0.08)
                }
                GradientStop {
                    position: 1.0
                    color: "transparent"
                }
            }
        }
    }
}
