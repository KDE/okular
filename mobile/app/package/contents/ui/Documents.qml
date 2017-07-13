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
import QtQuick.Controls 1.3
import QtQuick.Controls 2.0 as QQC2
import org.kde.kirigami 2.0 as Kirigami
import Qt.labs.folderlistmodel 2.1

Item {
    id: root
    anchors.fill: parent

    property Item view: filesView
    property alias contentY: filesView.contentY
    property alias contentHeight: filesView.contentHeight
    property alias model: filesView.model

    Item {
        id: toolBarContent
        width: root.width
        height: searchField.height + Kirigami.Units.gridUnit
        QQC2.TextField {
            id: searchField
            anchors.centerIn: parent
            focus: true
        }
    }

    Kirigami.Label {
        z: 2
        visible: filesView.count == 0
        anchors {
            fill: parent
            margins: Kirigami.Units.gridUnit
        }
        text: i18n("No Documents found. To start to read, put some files in the Documents folder of your device.")
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    ScrollView {
        anchors {
            top: toolBarContent.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        ListView {
            id: filesView
            anchors.fill: parent

            model:  FolderListModel {
                id: folderModel
                folder: userPaths.documents
                nameFilters: ["*.pdf", "*.txt", "*.chm", "*.epub"]
                showDirs: false
            }

            delegate: Kirigami.BasicListItem {
                label: model.fileName
                visible: model.fileName.indexOf(searchField.text) !== -1
                height: visible ? implicitHeight : 0
                onClicked: {
                    documentItem.path = model.filePath;
                    globalDrawer.close();
                    applicationWindow().controlsVisible = false;
                }
            }
        }
    }
}
