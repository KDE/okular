/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.17 as Kirigami

ThumbnailsBase {
    id: root
    model: documentItem.matchingPages

    header: Kirigami.AbstractApplicationHeader {
        topPadding: Kirigami.Units.smallSpacing / 2;
        bottomPadding: Kirigami.Units.smallSpacing / 2;
        rightPadding: Kirigami.Units.smallSpacing
        leftPadding: Kirigami.Units.smallSpacing

        width: root.width
        Kirigami.SearchField {
            id: searchField
            width: parent.width
            enabled: documentItem ? documentItem.supportsSearching : false
            onTextChanged: {
                if (text.length > 2) {
                    documentItem.searchText(text);
                } else {
                    documentItem.resetSearch();
                }
            }
        }
    }
}
