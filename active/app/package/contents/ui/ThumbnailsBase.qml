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
import org.kde.okular 0.1 as Okular
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.extras 0.1 as PlasmaExtras
import org.kde.plasma.mobilecomponents 0.1 as MobileComponents

PlasmaComponents.Page {
    id: root
    property alias contentY: resultsGrid.contentY
    property alias contentHeight: resultsGrid.contentHeight
    property alias model: resultsGrid.model
    signal pageClicked(int pageNumber)
    property Item view: resultsGrid

    anchors.fill: parent

    PlasmaExtras.ScrollArea {
        anchors.fill: parent

        GridView {
            id: resultsGrid
            anchors.fill: parent

            cellWidth: theme.defaultFont.mSize.width * 14
            cellHeight: theme.defaultFont.mSize.height * 12

            delegate: Item {
                width: resultsGrid.cellWidth
                height: resultsGrid.cellHeight
                property bool current: documentItem.currentPage == modelData
                onCurrentChanged: {
                    if (current) {
                        resultsGrid.currentIndex = index
                    }
                }
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
                        //value repeated to avoid binding loops
                        height: Math.round(theme.defaultFont.mSize.width * 10 / (implicitWidth/implicitHeight))
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

                            resourceBrowser.open = false
                            root.pageClicked(modelData)
                        }
                    }
                }
            }
            highlight: PlasmaComponents.Highlight {}
        }
    }
}
