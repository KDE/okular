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

import QtQuick 1.1
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.extras 0.1 as PlasmaExtras
import org.kde.plasma.mobilecomponents 0.1 as MobileComponents

PlasmaComponents.Page {
    property alias contentY: flickable.contentY
    property alias contentHeight: flickable.contentHeight

    tools: MobileComponents.ViewSearch {
        id: searchField
        anchors.centerIn: parent
        busy: documentItem.searchInProgress
    }
    PlasmaExtras.ScrollArea {
        anchors.fill: parent

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
