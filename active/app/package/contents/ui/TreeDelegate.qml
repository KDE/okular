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
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.core 0.1 as PlasmaCore


Column {
    id: treeDelegate
    property variant sourceModel
    property int rowIndex: index
    Row {
        visible: display.toLowerCase().indexOf(searchField.searchQuery.toLowerCase()) !== -1
        spacing: 30
        PlasmaComponents.Label {
            id: label
            text: display
        }
        PlasmaComponents.Label {
            id: pageNumber
            text: page
        }
    }
    Column {
        id: col
        x: 20
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
