/*
 *   Copyright 2012 by Marco Martin <mart@kde.org>
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

import QtQuick 2.2
import QtQuick.Controls 1.0
import org.kde.okular 2.0 as Okular

Item {
    width: 500
    height: 600
    Okular.DocumentItem {
        id: docItem
        path: "pageitem.cpp"
    }
    Okular.PageItem {
        id: page
        anchors.fill: parent
        document: docItem
    }
    Row {
        anchors {
            bottom: parent.bottom
            right: parent.right
        }
        Button {
            text: "prev"
            onClicked: page.pageNumber--
        }
        Button {
            text: "next"
            onClicked: page.pageNumber++
        }
    }
}
