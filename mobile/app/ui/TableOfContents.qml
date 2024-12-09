/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.17 as Kirigami
import org.kde.kitemmodels 1.0

ColumnLayout {
    id: root
    Kirigami.AbstractApplicationHeader {
        topPadding: Kirigami.Units.smallSpacing / 2;
        bottomPadding: Kirigami.Units.smallSpacing / 2;
        rightPadding: Kirigami.Units.smallSpacing
        leftPadding: Kirigami.Units.smallSpacing

        width: root.width
        Kirigami.SearchField {
            id: searchField
            width: parent.width
        }
    }
    QQC2.ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        ListView {
            model: KDescendantsProxyModel {
                model: documentItem.tableOfContents
                expandsByDefault: false
            }

            delegate: TreeItem {
                text: model.display
                onClicked: {
                    documentItem.currentPage = page - 1;
                    contextDrawer.drawerOpen = false;
                }
            }
        }
    }
}
