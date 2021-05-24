/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.1
import QtQuick.Controls 2.0 as QQC2
import org.kde.kirigami 2.8 as Kirigami

ThumbnailsBase {
    id: root
    model: documentItem.matchingPages
    padding: 0

    header: QQC2.ToolBar {
        id: toolBarContent
        padding: 0
        contentItem: Kirigami.SearchField {
            id: searchField
            placeholderText: i18n("Search...")
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
