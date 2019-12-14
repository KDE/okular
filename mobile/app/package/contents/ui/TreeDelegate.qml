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
import QtQuick.Controls 2.0 as QQC2
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.0 as Kirigami

Column {
    id: treeDelegate
    property variant sourceModel
    property int rowIndex: index
    width: parent.width

    property bool matches: display.toLowerCase().indexOf(searchField.text.toLowerCase()) !== -1

    Kirigami.BasicListItem {
        id: delegateArea
        height: matches ? implicitHeight : 0
        opacity: matches ? 1 : 0
        Behavior on opacity {
            NumberAnimation { duration: 250 }
        }
        Behavior on height {
            NumberAnimation { duration: 250 }
        }

        onClicked: {
            documentItem.currentPage = page-1
            contextDrawer.drawerOpen = false
        }

        label: display
        highlighted: highlight
        icon: highlight || highlightedParent ? (LayoutMirroring.enabled ? "arrow-left" : "arrow-right") : ""

        QQC2.Label {
            text: pageLabel ? pageLabel : page
            verticalAlignment: Text.AlignBottom
            Layout.rightMargin: Kirigami.Units.largeSpacing
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
