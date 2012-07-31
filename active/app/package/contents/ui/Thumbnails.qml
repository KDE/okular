/*
 *   Copyright 2012 Marco Martin <mart@kde.org>
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
import org.kde.okular 0.1 as Okular
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.extras 0.1 as PlasmaExtras
import org.kde.plasma.mobilecomponents 0.1 as MobileComponents

PlasmaComponents.Page {
    property alias contentY: resultsGrid.contentY

    anchors.fill: parent
    tools: Item {
        width: resultsGrid.width
        height: searchField.height
        MobileComponents.ViewSearch {
            id: searchField
            enabled: documentItem.supportsSearch
            anchors.centerIn: parent
            busy: documentItem.searchInProgress
            onSearchQueryChanged: {
                if (searchQuery.length > 2) {
                    documentItem.searchText(searchQuery)
                } else {
                    resultsGrid.currentIndex = pageArea.delegate.pageNumber
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
    PlasmaExtras.ScrollArea {
        anchors.fill: parent

        GridView {
            id: resultsGrid
            anchors.fill: parent

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

                            resourceBrowser.open = false
                        }
                    }
                }
            }
            highlight: PlasmaComponents.Highlight {}
        }
    }
}
