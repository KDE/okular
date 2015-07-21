/*
 *   Copyright 2015 Marco Martin <mart@kde.org>
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
import Qt.labs.folderlistmodel 2.1

PlasmaComponents.Page {
    id: root
    anchors.fill: parent
    property Item view: filesView
    property alias contentY: filesView.contentY
    property alias contentHeight: filesView.contentHeight
    property alias model: filesView.model

    tools: Item {
        id: toolBarContent
        width: root.width
        height: searchField.height
        MobileComponents.ViewSearch {
            id: searchField
            enabled: documentItem.supportsSearch
            anchors.centerIn: parent
            busy: documentItem.searchInProgress
            onSearchQueryChanged: {
                if (searchQuery.length > 2) {
                    filterModel.filterRegExp = ".*" + searchQuery + ".*";
                } else {
                    filterModel.filterRegExp = "";
                }
            }
        }
    }

    PlasmaExtras.ScrollArea {
        anchors.fill: parent
        GridView {
            id: filesView
            anchors.fill: parent
            cellWidth: units.gridUnit * 5
            cellHeight: units.gridUnit * 5
            model:  PlasmaCore.SortFilterModel {
                id: filterModel
                filterRole: "fileName"
                sourceModel: FolderListModel {
                    id: folderModel
                    folder: userPaths.documents
                    nameFilters: ["*.pdf", "*.txt", "*.chm", "*.epub"]
                    showDirs: false
                }
            }

            delegate: MouseArea {
                width: filesView.cellWidth
                height: filesView.cellHeight
                PlasmaCore.IconItem {
                    id: icon
                    width: units.gridUnit * 3
                    height: width
                    anchors {
                        top: parent.top
                        horizontalCenter: parent.horizontalCenter
                    }
                    //TODO: proper icons
                    source: "application-epub+zip"
                }
                PlasmaComponents.Label {
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: icon.bottom
                        bottom: parent.bottom
                    }
                    text: model.fileName
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                }
                onClicked: {
                    documentItem.path = model.filePath;
                    resourceBrowser.open = false;
                    mainTabBar.currentTab = thumbnailsButton;
                }
            }
        }
    }
}
