/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import org.kde.okular 2.0 as Okular

Item {
    width: 500
    height: 600
    Okular.DocumentItem {
        id: docItem
        url: "pageitem.cpp"
    }
    Okular.DocumentView {
        anchors.fill: parent
        document: docItem
    }
}
