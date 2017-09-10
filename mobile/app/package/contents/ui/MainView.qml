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
import QtQuick.Controls 1.3
import org.kde.okular 2.0 as Okular
import org.kde.kirigami 2.0 as Kirigami

Kirigami.Page {
    property alias document: pageArea.document
    leftPadding: 0
    topPadding: 0
    rightPadding: 0
    bottomPadding: 0

    actions.main: Kirigami.Action {
        iconName: "bookmarks-organize"
        checkable: true
        onCheckedChanged: pageArea.page.bookmarked = checked;
    }

    Okular.DocumentView {
        id: pageArea
        anchors.fill: parent

        onPageChanged: {
            bookmarkConnection.target = page
            actions.main.checked = page.bookmarked
        }
        onClicked: fileBrowserRoot.controlsVisible = !fileBrowserRoot.controlsVisible
    }

    Connections {
        id: bookmarkConnection
        target: pageArea.page
        onBookmarkedChanged: actions.main.checked = pageArea.page.bookmarked
    }
    ProgressBar {
        id: bar
        z: 99
        visible: applicationWindow().controlsVisible
        height: Kirigami.Units.smallSpacing
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        value: documentItem.pageCount != 0 ? ((documentItem.currentPage+1) / documentItem.pageCount) : 0
    }
}
