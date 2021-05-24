/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.7
import QtQuick.Dialogs 1.3 as QQD
import org.kde.okular 2.0 as Okular
import org.kde.kirigami 2.10 as Kirigami
import org.kde.okular.app 2.0

Kirigami.ApplicationWindow {
    id: fileBrowserRoot

    readonly property int columnWidth: Kirigami.Units.gridUnit * 13

    wideScreen: width > columnWidth * 5
    visible: true

    globalDrawer: Kirigami.GlobalDrawer {
        title: i18n("Okular")
        titleIcon: "okular"
        drawerOpen: false
        isMenu: true

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
                id: openDocumentAction
                text: i18n("Open...")
                icon.name: "document-open"
                onTriggered: {
                    fileDialog.open()
                }
            },
            Kirigami.Action {
                text: i18n("About")
                icon.name: "help-about-symbolic"
                onTriggered: fileBrowserRoot.pageStack.layers.push(aboutPage);
                enabled: fileBrowserRoot.pageStack.layers.depth === 1
            }
        ]
    }
    contextDrawer: OkularDrawer {
        contentItem.implicitWidth: columnWidth
        modal: !fileBrowserRoot.wideScreen
        onModalChanged: drawerOpen = !modal
        enabled: documentItem.opened && pageStack.layers.depth < 2
        handleVisible: enabled && pageStack.layers.depth < 2
    }

    title: documentItem.windowTitleForDocument ? documentItem.windowTitleForDocument : i18n("Okular")
    Okular.DocumentItem {
        id: documentItem
        onUrlChanged: { currentPage = 0 }
    }

    pageStack.initialPage: MainView {
        id: pageArea
        document: documentItem
        Kirigami.ColumnView.preventStealing: true
    }

    Component {
        id: aboutPage
        Kirigami.AboutPage {
            aboutData: about
        }
    }

    //FIXME: this is due to global vars being binded after the parse is done, do the 2 steps parsing
    Timer {
        interval: 100
        running: true
        onTriggered: {
            if (uri) {
                documentItem.url = uri
            }
        }
    }
}
