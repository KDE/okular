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

import QtQuick 2.7
import QtQuick.Dialogs 1.3 as QQD
import org.kde.okular 2.0 as Okular
import org.kde.kirigami 2.0 as Kirigami
import org.kde.okular.app 2.0

Kirigami.ApplicationWindow {
    id: fileBrowserRoot
    visible: true

    header: null
    globalDrawer: Kirigami.GlobalDrawer {
        title: i18n("Okular")
        titleIcon: "okular"

        QQD.FileDialog {
            id: fileDialog
            nameFilters: Okular.Okular.nameFilters
            folder: "file://" + userPaths.documents
            onAccepted: {
                documentItem.url = fileDialog.fileUrl
            }
        }

        actions: [
            Kirigami.Action {
                text: i18n("Open...")
                icon.name: "document-open"
                onTriggered: {
                    fileDialog.open()
                }
            }
        ]
    }
    contextDrawer: OkularDrawer {}

    title: documentItem.windowTitleForDocument
    Okular.DocumentItem {
        id: documentItem
        onUrlChanged: { currentPage = 0 }
    }

    MainView {
        id: pageArea
        anchors.fill: parent
        document: documentItem
    }

    //FIXME: this is due to global vars being binded after the parse is done, do the 2 steps parsing
    Timer {
        interval: 100
        running: true
        onTriggered: {
            if (uri) {
                documentItem.url = uri
            } else {
                globalDrawer.open();
            }
        }
    }
}
