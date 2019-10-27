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
import QtQuick.Controls 2.0 as QQC2
import org.kde.kirigami 2.0 as Kirigami

ThumbnailsBase {
    id: root
    model: documentItem.matchingPages

    QQC2.ToolBar {
        id: toolBarContent
        width: root.width
        QQC2.TextField {
            id: searchField
            width: parent.width
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
