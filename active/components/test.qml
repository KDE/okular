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
import org.kde.okular 0.1

Item {
    width: 400
    height: 200
    DocumentItem {
        id: documentItem
        //put the full path of a pdf file here to test
        path: "/path/to/pdf file"
    }
    Flickable {
        anchors {
            left: thumbnailsView.right
            top: parent.top
            bottom: parent.bottom
            right: parent.right
        }
        contentWidth: page.width
        contentHeight: page.height
        PageItem {
            id: page
            document: documentItem
            width: implicitWidth
            height: implicitHeight
        }
    }
    ListView {
        id: thumbnailsView
        width: 100
        height: parent.height
        model: documentItem.pageCount
        delegate: Rectangle {
            color: "red"
            width: parent.width
            height: width
            ThumbnailItem {
                id: thumbnail
                document: documentItem
                pageNumber: modelData
                width: parent.width
                height: parent.width / (implicitWidth/implicitHeight)
            }
            Text {
                text: modelData+1
            }
            Text {
                text: thumbnail.implicitWidth+"x"+thumbnail.implicitHeight
            }
            MouseArea {
                anchors.fill: parent
                onClicked: page.pageNumber = modelData
            }
        }
    }
}
