/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
