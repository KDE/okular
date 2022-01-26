/*
 *  SPDX-FileCopyrightText: 2020 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.17 as Kirigami
import org.kde.kitemmodels 1.0

/**
 * An item delegate for the TreeListView and TreeTableView components.
 *
 * It has the tree expander decoration but no content otherwise,
 * which has to be set as contentItem
 *
 */
QQC2.ItemDelegate {
    id: listItem

    property alias decoration: decoration

    width: ListView.view.width

    data: [
        TreeViewDecoration {
            id: decoration
            anchors {
                left: parent.left
                top:parent.top
                bottom: parent.bottom
                leftMargin: listItem.padding
            }
            parent: listItem
            parentDelegate: listItem
            model: listItem.ListView.view ? listItem.ListView.view.model :null
        }
    ]

    Keys.onLeftPressed: if (kDescendantExpandable && kDescendantExpanded) {
        decoration.model.collapseChildren(index);
    } else if (!kDescendantExpandable && kDescendantLevel > 0) {
        if (listItem.ListView.view) {
            const sourceIndex = decoration.model.mapToSource(decoration.model.index(index, 0));
            const newIndex = decoration.model.mapFromSource(sourceIndex.parent);
            listItem.listItem.ListView.view.currentIndex = newIndex.row;
        }
    }

    Keys.onRightPressed: if (kDescendantExpandable) {
        if (kDescendantExpanded && listItem.ListView.view) {
            ListView.view.incrementCurrentIndex();
        } else {
            decoration.model.expandChildren(index);
        }
    }

    onDoubleClicked: if (kDescendantExpandable) {
        decoration.model.toggleChildren(index);
    }

    contentItem: Kirigami.Heading {
        wrapMode: Text.Wrap
        text: listItem.text
        level: 5
        width: listItem.ListView.view.width - (decoration.width + listItem.padding * 3 + Kirigami.Units.smallSpacing)
    }

    leftInset: Qt.application.layoutDirection !== Qt.RightToLeft ? decoration.width + listItem.padding * 2 : 0
    leftPadding: Qt.application.layoutDirection !== Qt.RightToLeft ? decoration.width + listItem.padding * 2 : 0

    rightPadding: Qt.application.layoutDirection === Qt.RightToLeft ? decoration.width + listItem.padding * 2 : 0
    rightInset: Qt.application.layoutDirection === Qt.RightToLeft ? decoration.width + listItem.padding * 2 : 0
}

