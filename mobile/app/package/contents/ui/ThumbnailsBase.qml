/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import QtGraphicalEffects 1.15
import org.kde.okular 2.0 as Okular
import org.kde.kirigami 2.17 as Kirigami

ColumnLayout {
    id: root
    property alias model: resultsGrid.model
    property Item view: resultsGrid
    signal pageClicked(int pageNumber)

    property alias header: control.contentItem

    QQC2.Control {
        id: control
        Layout.fillWidth: true
        leftPadding: 0
        topPadding: 0
        bottomPadding: 0
        rightPadding: 0
    }

    QQC2.ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
        Kirigami.CardsListView {
            id: resultsGrid
            clip: true

            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                visible: model.length == 0
                width: parent.width - Kirigami.largeSpacing * 4
                text: i18n("No results found.")
            }

            delegate: Kirigami.AbstractCard {
                implicitWidth: root.width
                showClickFeedback: true
                readonly property real ratio: contentItem.implicitHeight/contentItem.implicitWidth
                implicitHeight: width * ratio
                contentItem: Okular.ThumbnailItem {
                    document: documentItem
                    pageNumber: modelData
                    Rectangle {
                        width: childrenRect.width
                        height: childrenRect.height
                        color: Kirigami.Theme.backgroundColor
                        radius: width
                        smooth: true
                        anchors {
                            top: parent.top
                            right: parent.right
                        }
                        QQC2.Label {
                            text: modelData + 1
                        }
                    }
                }
                onClicked: {
                    resultsGrid.currentIndex = index
                    documentItem.currentPage = modelData

                    contextDrawer.drawerOpen = false
                    root.pageClicked(modelData)
                }
            }
        }
    }
}
