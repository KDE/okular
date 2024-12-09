/*
 *  SPDX-FileCopyrightText: 2020 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Templates 2.15 as T
import org.kde.kitemmodels 1.0
import org.kde.kirigami 2.17 as Kirigami

/**
 * The tree expander decorator for item views.
 *
 * It will have a "> v" expander button graphics, and will have indentation on the left
 * depending on the level of the tree the item is in
 */
RowLayout {
    id: decorationLayout
    /**
     * The delegate this decoration will live in.
     * It needs to be assigned explicitly by the developer.
     */
    property T.ItemDelegate parentDelegate

    /**
     * The KDescendantsProxyModel the view is showing.
     * It needs to be assigned explicitly by the developer.
     */
    property KDescendantsProxyModel model

    property color decorationHighlightColor

    Layout.topMargin: -parentDelegate.topPadding
    Layout.bottomMargin: -parentDelegate.bottomPadding
    Repeater {
        model: kDescendantLevel-1
        delegate: Item {
            Layout.preferredWidth: controlRoot.width
            Layout.fillHeight: true

            Rectangle {
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: parent.top
                    bottom: parent.bottom
                }
                visible: kDescendantHasSiblings[modelData]
                color: Kirigami.Theme.textColor
                opacity: 0.5
                width: 1
            }
        }
    }
    T.Button {
        id: controlRoot
        Layout.preferredWidth: Kirigami.Units.gridUnit
        Layout.fillHeight: true
        enabled: kDescendantExpandable
        hoverEnabled: enabled
        onClicked: model.toggleChildren(index)
        contentItem: Item {
            id: styleitem
            implicitWidth: Kirigami.Units.gridUnit
            Rectangle {
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: parent.top
                    bottom: expander.visible ? expander.top : parent.verticalCenter
                }
                color: Kirigami.Theme.textColor
                opacity: 0.5
                width: 1
            }
            Kirigami.Icon {
                id: expander
                anchors.centerIn: parent
                width: Kirigami.Units.iconSizes.small
                height: width
                source: kDescendantExpanded ? "go-down-symbolic" : (Qt.application.layoutDirection == Qt.RightToLeft ? "go-previous-symbolic" : "go-next-symbolic")
                isMask: true
                color: controlRoot.hovered ? decorationLayout.decorationHighlightColor ? decorationLayout.decorationHighlightColor :
                Kirigami.Theme.highlightColor : Kirigami.Theme.textColor
                Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration; easing.type: Easing.InOutQuad } }
                visible: kDescendantExpandable
            }
            Rectangle {
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: expander.visible ? expander.bottom : parent.verticalCenter
                    bottom: parent.bottom
                }
                visible: kDescendantHasSiblings[kDescendantHasSiblings.length - 1]
                color: Kirigami.Theme.textColor
                opacity: 0.5
                width: 1
            }
            Rectangle {
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: expander.visible ? expander.right : parent.horizontalCenter
                    right: parent.right
                }
                color: Kirigami.Theme.textColor
                opacity: 0.5
                height: 1
            }
        }
        background: Item {}
    }
}
