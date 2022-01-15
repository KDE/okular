/*
    SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.2
import QtQuick.Controls 2.2 as QQC2
import org.kde.okular 2.0
import org.kde.kirigami 2.15 as Kirigami
import "./private"

/**
 * A touchscreen optimized view for a document
 * 
 * It supports changing pages by a swipe gesture, pinch zoom
 * and flicking to scroll around
 */
ListView {
    id: root
    property DocumentItem document

    /**
     * @property PageItem page
     */
    property var page: root.currentItem ? root.currentItem.pageItem : null

    signal clicked

    model: document.pageCount

    readonly property bool isCurrentItemInteractive: currentItem ? currentItem.interactive : false

    delegate: ZoomArea {
        width: root.width
        height: root.height
        implicitContentWidth: pageItem.implicitWidth
        implicitContentHeight: pageItem.implicitHeight

        minimumZoomSize: 8
        maximumZoomFactor: 100

        property alias pageItem: page

        PageItem {
            id: page
            document: root.document
            flickable: root
            anchors.fill: parent
            pageNumber: modelData
        }
    }

    currentIndex: document.currentPage
    snapMode: ListView.SnapOneItem
    orientation: Qt.Horizontal
    highlightRangeMode: ListView.StrictlyEnforceRange
    pixelAligned: true
    highlightMoveDuration: 0
    interactive: !isCurrentItemInteractive
}
