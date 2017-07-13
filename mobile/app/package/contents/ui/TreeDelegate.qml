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
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kirigami 2.0 as Kirigami

Column {
    id: treeDelegate
    property variant sourceModel
    property int rowIndex: index
    width: parent.width

    property bool matches: display.toLowerCase().indexOf(searchField.text.toLowerCase()) !== -1


    MouseArea {
        id: delegateArea
        width: parent.width
        height: matches ? label.height : 0
        opacity: matches ? 1 : 0
        Behavior on opacity {
            NumberAnimation {
                duration: 250
            }
        }


        onClicked: {
            documentItem.currentPage = page-1

            contextDrawer.opened = false
        }

        QIconItem {
            id: icon
            icon: decoration
            width: theme.smallIconSize
            height: width
            anchors.verticalCenter: parent.verticalCenter
            x: units.largeSpacing
        }
        Kirigami.Label {
            id: label
            text: display
            verticalAlignment: Text.AlignBottom
            anchors.left: icon.right
        }
        //there isn't a sane way to do a dotted line in QML
        Rectangle {
            color: theme.textColor
            opacity: 0.3
            height: 1
            anchors {
                bottom: parent.bottom
                left: label.right
                right: pageNumber.left
            }
        }
        Kirigami.Label {
            id: pageNumber
            text: pageLabel ? pageLabel : page
            anchors.right: parent.right
            verticalAlignment: Text.AlignBottom
            anchors.rightMargin: units.largeSpacing
        }
    }
    Column {
        id: col
        x: 20
        width: parent.width - 20
        property variant model: childrenModel
        Repeater {
            id: rep
            model: VisualDataModel {
                id: childrenModel
                model: documentItem.tableOfContents
            }
        }
    }
    onParentChanged: {
        if (treeDelegate.parent && treeDelegate.parent.model) {
            sourceModel = treeDelegate.parent.model
        }

        childrenModel.rootIndex = sourceModel.modelIndex(index)

        if (model.hasModelChildren) {
            childrenModel.delegate = Qt.createComponent("TreeDelegate.qml")
        }
    }
}
