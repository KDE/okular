/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.1
import QtQuick.Controls 2.2 as QQC2
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.0 as Kirigami

Kirigami.ScrollablePage {
    id: root

    header: QQC2.ToolBar {
        id: toolBarContent
        width: root.width
        QQC2.TextField {
            id: searchField
            width: parent.width
            placeholderText: i18n("Search...")
        }
    }
    ColumnLayout {
        spacing: 0
        Repeater {
            model: VisualDataModel {
                id: tocModel
                model: documentItem.tableOfContents
                delegate: TreeDelegate {
                    Layout.fillWidth: true
                    sourceModel: tocModel
                }
            }
        }
    }
}
