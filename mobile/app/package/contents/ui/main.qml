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

ApplicationWindow {
    id: fileBrowserRoot
    objectName: "fileBrowserRoot"
    visible: true

    width: 360
    height: 360

    /*TODO: port ResourceInstance
    PlasmaExtras.ResourceInstance {
        id: resourceInstance
        uri: documentItem.path
    }*/

    statusBar: PlasmaComponents.ToolBar {
        id: toolbar
        tools: PlasmaComponents.ToolBarLayout {
            //TODO: those buttons should support drag to open the menus as well
            PlasmaComponents.ToolButton {
                id: openButton
                iconSource: "document-open"
                checkable: true
                onCheckedChanged: {
                    mainStack.currentPage.splitDrawerOpen = checked
                    if (checked) {
                        mainStack.currentPage.overlayDrawerOpen = false;
                    }
                }
            }
            PlasmaComponents.ToolButton {
                id: bookmarkButton
                iconSource: "bookmarks-organize"
                checkable: true
                onCheckedChanged: {
                    mainStack.currentPage.bookmarked = checked
                }
            }
            PlasmaComponents.ToolButton {
                id: exploreButton
                iconSource: "view-list-icons"
                checkable: true
                onCheckedChanged: {
                    mainStack.currentPage.overlayDrawerOpen = checked
                    if (checked) {
                        mainStack.currentPage.splitDrawerOpen = false;
                    }
                }
            }
        }
        Connections {
            target: mainStack.currentPage
            onSplitDrawerOpenChanged: {
                openButton.checked = mainStack.currentPage.splitDrawerOpen;
            }
            onOverlayDrawerOpenChanged: {
                exploreButton.checked = mainStack.currentPage.overlayDrawerOpen;
            }
            onBookmarkedChanged: {
                bookmarkButton.checked = mainStack.currentPage.bookmarked
            }
        }
        PlasmaComponents.ProgressBar {
            id: bar
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.top
            }
            height: units.smallSpacing
            value: documentItem.pageCount != 0 ? (documentItem.currentPage / documentItem.pageCount) : 0
        }
    }

    Okular.DocumentItem {
        id: documentItem
        onWindowTitleForDocumentChanged: {
            fileBrowserRoot.title = windowTitleForDocument
        }
    }

    PlasmaComponents.PageStack {
        id: mainStack
        clip: false
    }

    //FIXME: this is due to global vars being binded after the parse is done, do the 2 steps parsing
    Timer {
        interval: 100
        running: true
        onTriggered: {
            if (commandlineArguments.length > 0) {
                documentItem.path = commandlineArguments[0]
            }

            var browser = mainStack.push(Qt.createComponent("Browser.qml"))
            if (commandlineArguments.length == 0) {
                browser.open = true;
            }
        }
    }
}
