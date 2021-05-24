/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.1
import QtQuick.Controls 2.3 as QQC2
import QtGraphicalEffects 1.0
import org.kde.okular 2.0 as Okular
import org.kde.kirigami 2.5 as Kirigami

Kirigami.ScrollablePage {
    id: root
    property alias model: resultsGrid.model
    property Item view: resultsGrid
    signal pageClicked(int pageNumber)

    contentItem: Kirigami.CardsListView {
        id: resultsGrid
        clip: true

        QQC2.Label {
            anchors.centerIn: parent
            visible: model.length == 0
            text: i18n("No results found.")
        }

        delegate: Kirigami.AbstractCard {
            implicitWidth: root.width
            highlighted: delegateRecycler && delegateRecycler.GridView.isCurrentItem
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
