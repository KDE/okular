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
