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
import org.kde.okular 2.0 as Okular
import QtQuick.Controls 1.3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

MobileComponents.ApplicationWindow {
    id: fileBrowserRoot
    objectName: "fileBrowserRoot"
    visible: true

    /*TODO: port ResourceInstance
    PlasmaExtras.ResourceInstance {
        id: resourceInstance
        uri: documentItem.path
    }*/

    globalDrawer: MobileComponents.OverlayDrawer {
        edge: Qt.LeftEdge
        contentItem: Documents {
            implicitWidth: units.gridUnit * 20
        }
    }
    contextDrawer: OkularDrawer {}
    actionButton.iconSource: "bookmarks-organize"
    actionButton.checkable: true
    actionButton.onCheckedChanged: pageArea.page.bookmarked = actionButton.checked;
    PlasmaComponents.ProgressBar {
        id: bar
        z: 99
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: units.smallSpacing
        value: documentItem.pageCount != 0 ? (documentItem.currentPage / documentItem.pageCount) : 0
    }

    Okular.DocumentItem {
        id: documentItem
        onWindowTitleForDocumentChanged: {
            fileBrowserRoot.title = windowTitleForDocument
        }
    }

    Okular.DocumentView {
        id: pageArea
        document: documentItem
        anchors.fill: parent

        onPageChanged: {
            bookmarkConnection.target = page
            actionButton.checked = page.bookmarked
        }
        onClicked: actionButton.toggleVisibility();
    }
    Connections {
        id: bookmarkConnection
        target: pageArea.page
        onBookmarkedChanged: actionButton.checked = page.bookmarked
    }

    //FIXME: this is due to global vars being binded after the parse is done, do the 2 steps parsing
    Timer {
        interval: 100
        running: true
        onTriggered: {
            if (commandlineArguments.length > 0) {
                documentItem.path = commandlineArguments[0]
            }

            if (commandlineArguments.length == 0) {
                globalDrawer.opened = true;
            }
        }
    }
}
