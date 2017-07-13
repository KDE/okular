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
import QtQuick.Controls 1.3
import QtGraphicalEffects 1.0
import org.kde.okular 2.0 as Okular
import org.kde.kirigami 2.0 as Kirigami

Kirigami.Page {
    id: root
    leftPadding: 0
    topPadding: 0
    rightPadding: 0
    bottomPadding: 0
    property alias contentY: resultsGrid.contentY
    property alias contentHeight: resultsGrid.contentHeight
    property alias model: resultsGrid.model
    signal pageClicked(int pageNumber)
    property Item view: resultsGrid

    ScrollView {
        anchors {
            fill: parent
            topMargin: Kirigami.Units.gridUnit * 2
        }

        GridView {
            id: resultsGrid
            anchors.fill: parent

            cellWidth: Math.floor(width / Math.floor(width / (Kirigami.Units.gridUnit * 8)))
            cellHeight: Math.floor(cellWidth * 1.6)

            delegate: Item {
                id: delegate
                width: resultsGrid.cellWidth
                height: resultsGrid.cellHeight
                property bool current: documentItem.currentPage == modelData
                onCurrentChanged: {
                    if (current) {
                        resultsGrid.currentIndex = index
                    }
                }
                Rectangle {
                    anchors.centerIn: parent
                    width: thumbnail.width + Kirigami.Units.smallSpacing * 2
                    //FIXME: why bindings with thumbnail.height doesn't work?
                    height: thumbnail.height + Kirigami.Units.smallSpacing * 2
                    Okular.ThumbnailItem {
                        id: thumbnail
                        x: Kirigami.Units.smallSpacing
                        y: Kirigami.Units.smallSpacing
                        document: documentItem
                        pageNumber: modelData
                        width: delegate.width - Kirigami.Units.smallSpacing * 2 - Kirigami.Units.gridUnit
                        //value repeated to avoid binding loops
                        height: Math.round(width / (implicitWidth/implicitHeight))
                        Rectangle {
                            width: childrenRect.width
                            height: childrenRect.height
                            color: Kirigami.Theme.backgroundColor
                            radius: width
                            smooth: true
                            anchors {
                                bottom: parent.bottom
                                right: parent.right
                            }
                            Kirigami.Label {
                                text: modelData + 1
                            }
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            resultsGrid.currentIndex = index
                            documentItem.currentPage = modelData

                            contextDrawer.opened = false
                            root.pageClicked(modelData)
                        }
                    }
                }
                layer.enabled: true
                layer.effect: DropShadow {
                    horizontalOffset: 0
                    verticalOffset: 0
                    radius: Math.ceil(Kirigami.Units.gridUnit * 0.8)
                    samples: 32
                    color: Qt.rgba(0, 0, 0, 0.5)
                }
            }
            highlight: Rectangle {
                color: Kirigami.Theme.highlightColor
                opacity: 0.4
            }
        }
    }
}
