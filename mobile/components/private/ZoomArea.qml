/*
 * SPDX-FileCopyrightText: 2015 Vishesh Handa <vhanda@kde.org>
 * SPDX-FileCopyrightText: 2017 Atul Sharma <atulsharma406@gmail.com>
 * SPDX-FileCopyrightText: 2017 Marco Martin <mart@kde.org>
 * SPDX-FileCopyrightText: 2021 Noah Davis <noahadvs@gmail.com>
 * SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

import QtQuick 2.15
import QtQml 2.15
import org.kde.kirigami 2.15 as Kirigami

MouseArea {
    id: root
    readonly property bool interactive: Math.floor(contentItem.width) > root.width || Math.floor(contentItem.height) > root.height
    property bool dragging: root.drag.active || pinchArea.pinch.active

    /**
     * Properties used for contentItem manipulation.
     */
    readonly property alias contentItem: contentItem
    default property alias contentData: contentItem.data
    property alias contentChildren: contentItem.children
    // NOTE: Unlike Flickable, contentX and contentY do not have reversed signs.
    // NOTE: contentX and contentY can be NaN/undefined sometimes even when
    // contentItem.x and contentItem.y aren't and I'm not sure why.
    property alias contentX: contentItem.x
    property alias contentY: contentItem.y
    property alias contentWidth: contentItem.width
    property alias contentHeight: contentItem.height
    property alias implicitContentWidth: contentItem.implicitWidth
    property alias implicitContentHeight: contentItem.implicitHeight
    readonly property rect defaultContentRect: {
        const size = fittedContentSize(contentItem.implicitWidth, contentItem.implicitHeight)
        return Qt.rect(centerContentX(size.width), centerContentY(size.height), size.width, size.height)
    }
    readonly property real contentAspectRatio: contentItem.implicitWidth / contentItem.implicitHeight
    readonly property real viewAspectRatio: root.width / root.height
    // Should be the same for both width and height
    readonly property real zoomFactor: contentItem.width / contentItem.implicitWidth

    // Minimum is a size because a factor doesn't necessarily
    // limit based on what is visible on the user's screen.
    // NOTE: if the implicit content size is smaller, that will be used instead.
    property int minimumZoomSize: 8
    // Maximum is a factor because scaling up in proportion to the
    // original size is the most common behavior for zoom controls.
    property real maximumZoomFactor: 100

    // Fit to root unless arguments are smaller than the size of root.
    // Returning size instead of using separate width and height functions
    // since they both need to be calculated together.
    function fittedContentSize(w, h) {
        const factor = root.contentAspectRatio >= root.viewAspectRatio ?
            root.width / w : root.height / h
        if (w > root.width || h > root.height) {
            w = w * factor
            h = h * factor
        }
        return Qt.size(w, h)
    }

    // Get the X value that would center the contentItem with the given content width.
    function centerContentX(cWidth = contentItem.width) {
        return Math.round((root.width - cWidth) / 2)
    }

    // Get the Y value that would center the contentItem with the given content height.
    function centerContentY(cHeight = contentItem.height) {
        return Math.round((root.height - cHeight) / 2)
    }

    // Right side of content touches right side of root.
    function minContentX(cWidth = contentItem.width) {
        return cWidth > root.width ? root.width - cWidth : centerContentX(cWidth)
    }
    // Left side of content touches left side of root.
    function maxContentX(cWidth = contentItem.width) {
        return cWidth > root.width ? 0 : centerContentX(cWidth)
    }
    // Bottom side of content touches bottom side of root.
    function minContentY(cHeight = contentItem.height) {
        return cHeight > root.height ? root.height - cHeight : centerContentY(cHeight)
    }
    // Top side of content touches top side of root.
    function maxContentY(cHeight = contentItem.height) {
        return cHeight > root.height ? 0 : centerContentY(cHeight)
    }

    function bound(min, value, max) {
        return Math.min(Math.max(min, value), max)
    }

    function boundedContentWidth(newWidth) {
        return bound(Math.min(contentItem.implicitWidth, root.minimumZoomSize),
                     newWidth,
                     contentItem.implicitWidth * root.maximumZoomFactor)
    }
    function boundedContentHeight(newHeight) {
        return bound(Math.min(contentItem.implicitHeight, root.minimumZoomSize),
                     newHeight,
                     contentItem.implicitHeight * root.maximumZoomFactor)
    }

    function boundedContentX(newX, cWidth = contentItem.width) {
        return Math.round(bound(minContentX(cWidth), newX, maxContentX(cWidth)))
    }
    function boundedContentY(newY, cHeight = contentItem.height) {
        return Math.round(bound(minContentY(cHeight), newY, maxContentY(cHeight)))
    }

    function heightForWidth(w = contentItem.width) {
        return w / root.contentAspectRatio
    }
    function widthForHeight(h = contentItem.height) {
        return h * root.contentAspectRatio
    }

    function addContentSize(value, w = contentItem.width, h = contentItem.height) {
        if (root.contentAspectRatio >= 1) {
            w = boundedContentWidth(w + value)
            h = heightForWidth(w)
        } else {
            h = boundedContentHeight(h + value)
            w = widthForHeight(h)
        }
        return Qt.size(w, h)
    }

    function multiplyContentSize(value, w = contentItem.width, h = contentItem.height) {
        if (root.contentAspectRatio >= 1) {
            w = boundedContentWidth(w * value)
            h = heightForWidth(w)
        } else {
            h = boundedContentHeight(h * value)
            w = widthForHeight(h)
        }
        return Qt.size(w, h)
    }

    /**
     * Basic formula: (qreal) steps * singleStep * wheelScrollLines
     * 120 delta units == 1 step.
     * singleStep is the step amount in pixels.
     * wheelScrollLines is the step multiplier.
     *
     * There is no real standard for scroll speed.
     * - QScrollArea uses `singleStep = 20`
     * - QGraphicsView uses `singleStep = dimension / 20`
     * - Kirigami WheelHandler uses `singleStep = delta / 8`
     * - Some apps use `singleStep = QFontMetrics::height()`
     */
    function angleDeltaToPixels(delta, dimension) {
        const singleStep = dimension !== undefined ? dimension / 20 : 20
        return delta / 120 * singleStep * Qt.styleHints.wheelScrollLines
    }

    clip: true
    acceptedButtons: root.interactive ? Qt.LeftButton | Qt.MiddleButton : Qt.LeftButton
    cursorShape: if (root.interactive) {
        return pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
    } else {
        return Qt.ArrowCursor
    }

    drag {
        axis: Drag.XAndYAxis
        target: root.interactive ? contentItem : undefined
        minimumX: root.minContentX(contentItem.width)
        maximumX: root.maxContentX(contentItem.width)
        minimumY: root.minContentY(contentItem.height)
        maximumY: root.maxContentY(contentItem.height)
    }

    Item {
        id: contentItem
        width: root.defaultContentRect.width
        height: root.defaultContentRect.height
        x: root.defaultContentRect.x
        y: root.defaultContentRect.y
    }

    // Auto center
    Binding {
        // we tried using delayed here but that causes flicker issues
        target: contentItem; property: "x"
        when: contentItem.implicitWidth > 0 && Math.floor(contentItem.width) <= root.width && !root.dragging
        value: root.centerContentX(contentItem.width)
        restoreMode: Binding.RestoreNone
    }
    Binding {
        target: contentItem; property: "y"
        when: contentItem.implicitHeight > 0 && Math.floor(contentItem.height) <= root.height && !root.dragging
        value: root.centerContentY(contentItem.height)
        restoreMode: Binding.RestoreNone
    }

    onWidthChanged: if (contentItem.width > width) {
        contentItem.x = boundedContentX(contentItem.x)
    }
    onHeightChanged: if (contentItem.height > height) {
        contentItem.y = boundedContentY(contentItem.y)
    }

    PinchArea {
        id: pinchArea
        property real initialWidth: 0
        property real initialHeight: 0
        parent: root
        anchors.fill: parent
        enabled: root.enabled
        pinch {
            dragAxis: Pinch.XAndYAxis
            target: root.drag.target
            minimumX: root.drag.minimumX
            maximumX: root.drag.maximumX
            minimumY: root.drag.minimumY
            maximumY: root.drag.maximumY
            minimumScale: 1
            maximumScale: 1
            minimumRotation: 0
            maximumRotation: 0
        }

        onPinchStarted: {
            initialWidth = contentItem.width
            initialHeight = contentItem.height
        }

        onPinchUpdated: {
            // adjust content pos due to drag
            //contentItem.x = pinch.previousCenter.x - pinch.center.x + contentItem.x
            //contentItem.y = pinch.previousCenter.y - pinch.center.y + contentItem.y

            // resize content
            const newSize = root.multiplyContentSize(pinch.scale, initialWidth, initialHeight)
            contentItem.width = newSize.width
            contentItem.height = newSize.height
            //contentItem.x = boundedContentX(contentItem.x - pinch.center.x)
            //contentItem.y = boundedContentY(contentItem.y - pinch.center.y)
        }
    }

    onDoubleClicked: if (mouse.button === Qt.LeftButton) {
        if (Kirigami.Settings.isMobile) { applicationWindow().controlsVisible = false }
        if (contentItem.width !== root.defaultContentRect.width || contentItem.height !== root.defaultContentRect.height) {
            contentItem.width = Qt.binding(() => root.defaultContentRect.width)
            contentItem.height = Qt.binding(() => root.defaultContentRect.height)
        } else {
            const cX = contentItem.x, cY = contentItem.y
            contentItem.width = root.defaultContentRect.width * 2
            contentItem.height = root.defaultContentRect.height * 2
            // content position * factor - mouse position
            contentItem.x = root.boundedContentX(cX * 2 - mouse.x, contentItem.width)
            contentItem.y = root.boundedContentY(cY * 2 - mouse.y, contentItem.height)
        }
    }
    onWheel: {
        if (wheel.modifiers & Qt.ControlModifier || wheel.modifiers & Qt.ShiftModifier) {
            const pixelDeltaX = wheel.pixelDelta.x !== 0 ?
                wheel.pixelDelta.x : angleDeltaToPixels(wheel.angleDelta.x, root.width)
            const pixelDeltaY = wheel.pixelDelta.y !== 0 ?
                wheel.pixelDelta.y : angleDeltaToPixels(wheel.angleDelta.y, root.height)
            if (pixelDeltaX !== 0 && pixelDeltaY !== 0) {
                contentItem.x = root.boundedContentX(pixelDeltaX + contentItem.x)
                contentItem.y = root.boundedContentY(pixelDeltaY + contentItem.y)
            } else if (pixelDeltaX !== 0 && pixelDeltaY === 0) {
                contentItem.x = root.boundedContentX(pixelDeltaX + contentItem.x)
            } else if (pixelDeltaX === 0 && pixelDeltaY !== 0 && wheel.modifiers & Qt.ShiftModifier) {
                contentItem.x = root.boundedContentX(pixelDeltaY + contentItem.x)
            } else {
                contentItem.y = root.boundedContentY(pixelDeltaY + contentItem.y)
            }
        } else {
            let factor = 1 + Math.abs(wheel.angleDelta.y / 600)
            if (wheel.angleDelta.y < 0) {
                factor = 1 / factor
            }
            const oldRect = Qt.rect(contentItem.x, contentItem.y, contentItem.width, contentItem.height)
            const newSize = root.multiplyContentSize(factor)
            // round to default size if within ±1
            if ((newSize.height > root.defaultContentRect.height - 1
                && newSize.height < root.defaultContentRect.height + 1)
             || (newSize.width > root.defaultContentRect.width - 1
                && newSize.width < root.defaultContentRect.width + 1)
            ) {
                contentItem.width = root.defaultContentRect.width
                contentItem.height = root.defaultContentRect.height
            } else {
                contentItem.width = newSize.width
                contentItem.height = newSize.height
            }
            if (root.interactive) {
                contentItem.x = root.boundedContentX(wheel.x - contentItem.width * ((wheel.x - oldRect.x)/oldRect.width))
                contentItem.y = root.boundedContentY(wheel.y - contentItem.height * ((wheel.y - oldRect.y)/oldRect.height))
            }
        }
    }
}
