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

MobileComponents.Page {
    id: root
    anchors.fill: parent
    color: theme.viewBackgroundColor
    visible: true
    property Item view: filesView
    property alias contentY: filesView.contentY
    property alias contentHeight: filesView.contentHeight
    property alias model: filesView.model

    tools: Item {
        id: toolBarContent
        width: root.width
        height: searchField.height
        PlasmaComponents.TextField {
            id: searchField
            anchors.centerIn: parent
            onTextChanged: {
                if (text.length > 2) {
                    filterModel.filterRegExp = ".*" + text + ".*";
                } else {
                    filterModel.filterRegExp = "";
                }
            }
        }
    }

    MobileComponents.Label {
        z: 2
        visible: filesView.count == 0
        anchors {
            fill: parent
            margins: MobileComponents.Units.gridUnit
        }
        text: i18n("No Documents found. To start to read, put some files in the Documents folder of your device.")
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    PlasmaExtras.ScrollArea {
        anchors.fill: parent
        ListView {
            id: filesView
            anchors.fill: parent

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

            delegate: MobileComponents.ListItem {
                enabled: true
                PlasmaComponents.Label {
                    text: model.fileName
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                }
                onClicked: {
                    documentItem.path = model.filePath;
                    globalDrawer.opened = false;
                    mainTabBar.currentTab = thumbnailsButton;
                }
            }
        }
    }
}
