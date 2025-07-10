/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtCore
import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Dialogs as QQD
import org.kde.okular 2.0 as Okular
import org.kde.kirigami 2.17 as Kirigami
import org.kde.kirigamiaddons.formcard 1.0 as FormCard
import org.kde.okular.app

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
            currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
            onAccepted: {
                documentItem.url = fileDialog.selectedFile
            }
        }

        actions: [
            Kirigami.Action {
                id: openDocumentAction
                text: i18n("Open…")
                icon.name: "document-open"
                onTriggered: {
                    fileDialog.open()
                }
            },
            Kirigami.Action {
                text: i18n("About")
                icon.name: "help-about-symbolic"
                onTriggered: fileBrowserRoot.pageStack.layers.push(Qt.createComponent("org.kde.kirigamiaddons.formcard", "AboutPage"));
                enabled: fileBrowserRoot.pageStack.layers.depth === 1
            }
        ]
    }
    contextDrawer: OkularDrawer {
        width: columnWidth
        contentItem.implicitWidth: columnWidth
        modal: !fileBrowserRoot.wideScreen
        onModalChanged: drawerOpen = !modal
        onEnabledChanged: drawerOpen = enabled && !modal
        enabled: documentItem.opened && pageStack.layers.depth < 2
        handleVisible: enabled && pageStack.layers.depth < 2
    }

    title: documentItem.windowTitleForDocument ? documentItem.windowTitleForDocument : i18n("Okular")
    Okular.DocumentItem {
        id: documentItem
        onUrlChanged: { currentPage = 0 }

        onNeedsPasswordChanged: {
            if (needsPassword) {
                passwordDialog.open();
            }
        }
    }

    pageStack.initialPage: MainView {
        id: mainView
        document: documentItem
        Kirigami.ColumnView.preventStealing: true
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

    QQC2.Dialog {
        id: passwordDialog
        focus: true
        anchors.centerIn: parent
        title: i18n("Password Needed")
        contentItem: Kirigami.PasswordField {
            id: pwdField
            onAccepted: passwordDialog.accept();
            focus: true
        }
        standardButtons: QQC2.Dialog.Ok | QQC2.Dialog.Cancel

        onAccepted: documentItem.setPassword(pwdField.text);
        onRejected: documentItem.url = "";
    }
}
