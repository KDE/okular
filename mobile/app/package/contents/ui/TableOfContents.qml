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
import QtQuick.Controls 2.2 as QQC2
import org.kde.kirigami 2.0 as Kirigami

Kirigami.Page {
    id: root
    leftPadding: 0
    topPadding: 0
    rightPadding: 0
    bottomPadding: 0
    property alias tocContentY: flickable.contentY
    property alias tocContentHeight: flickable.contentHeight

    QQC2.ToolBar {
        id: toolBarContent
        width: root.width
        QQC2.TextField {
            id: searchField
            width: parent.width
            placeholderText: i18n("Search...")
        }
    }
    QQC2.ScrollView {
        anchors {
            left: parent.left
            top: toolBarContent.bottom
            right: parent.right
            bottom: parent.bottom
        }

        Flickable {
            id: flickable
            anchors.fill: parent
            contentWidth: width
            contentHeight: treeView.height
            Column {
                id: treeView
                width: flickable.width
                Repeater {
                    model: VisualDataModel {
                        id: tocModel
                        model: documentItem.tableOfContents
                        delegate: TreeDelegate {
                            sourceModel: tocModel
                            width: treeView.width
                        }
                    }
                }
            }
        }
    }
}
