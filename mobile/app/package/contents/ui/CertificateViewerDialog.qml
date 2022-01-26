/*
 *   SPDX-FileCopyrightText: 2022 Albert Astals Cid <aacid@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Dialogs 1.3 as QQD
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.17 as Kirigami
import org.kde.okular 2.0

Kirigami.OverlaySheet
{
    id: root

    property var certificateModel

    title: i18n("Certificate Viewer")

    ColumnLayout {
        // Without this the width is unreasonably narrow, potentially
        // https://invent.kde.org/frameworks/kirigami/-/merge_requests/487 fixes it
        // check when a kirigami with that is required as minimum version
        Layout.preferredWidth: Math.min(Window.window.width, 360)

        QQC2.GroupBox {
            Layout.fillWidth: true

            title: i18n("Issued By")

            Kirigami.FormLayout {
                width: parent.width
                QQC2.Label {
                    Kirigami.FormData.label: i18n("Common Name:")
                    text: certificateModel.propertyVisibleValue(CertificateModel.IssuerName)
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    Kirigami.FormData.label: i18n("EMail:")
                    text: certificateModel.propertyVisibleValue(CertificateModel.IssuerEmail)
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    Kirigami.FormData.label: i18n("Organization:")
                    text: certificateModel.propertyVisibleValue(CertificateModel.IssuerOrganization)
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }
        }

        QQC2.GroupBox {
            Layout.fillWidth: true

            title: i18n("Issued To")

            Kirigami.FormLayout {
                width: parent.width
                QQC2.Label {
                    Kirigami.FormData.label: i18n("Common Name:")
                    text: certificateModel.propertyVisibleValue(CertificateModel.SubjectName)
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    Kirigami.FormData.label: i18n("EMail:")
                    text: certificateModel.propertyVisibleValue(CertificateModel.SubjectEmail)
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    Kirigami.FormData.label: i18n("Organization:")
                    text: certificateModel.propertyVisibleValue(CertificateModel.SubjectOrganization)
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }
        }

        QQC2.GroupBox {
            Layout.fillWidth: true

            title: i18n("Validity")

            Kirigami.FormLayout {
                width: parent.width
                QQC2.Label {
                    Kirigami.FormData.label: i18n("Issued On:")
                    text: certificateModel.propertyVisibleValue(CertificateModel.IssuedOn)
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    Kirigami.FormData.label: i18n("Expires On:")
                    text: certificateModel.propertyVisibleValue(CertificateModel.ExpiresOn)
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }
        }

        QQC2.GroupBox {
            Layout.fillWidth: true

            title: i18n("Fingerprints")

            Kirigami.FormLayout {
                width: parent.width
                QQC2.Label {
                    Kirigami.FormData.label: i18n("SHA-1 Fingerprint:")
                    text: certificateModel.propertyVisibleValue(CertificateModel.Sha1)
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    Kirigami.FormData.label: i18n("SHA-256 Fingerprint:")
                    text: certificateModel.propertyVisibleValue(CertificateModel.Sha256)
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }
        }

        QQC2.DialogButtonBox {
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.fillWidth: true

            QQC2.Button {
                QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.ActionRole
                text: i18n("Export...")
                onClicked: fileDialog.open()
            }

            QQC2.Button {
                QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.DestructiveRole
                text: i18n("Close")
                icon.name: "dialog-close"
                onClicked: root.close()
            }
        }

        QQD.FileDialog {
            id: fileDialog
            nameFilters: i18n("Certificate File (*.cer)")
            folder: "file://" + userPaths.documents
            selectExisting: false
            onAccepted: {
                if (!certificateModel.exportCertificateTo(fileDialog.fileUrl)) {
                    errorDialog.open();
                }
            }
        }

        // TODO Use Kirigami.PromptDialog when we depend on KF >= 5.89
        // this way we can probably remove that ridiculous z value
        QQC2.Dialog {
            id: errorDialog
            z: 200
            title: i18n("Error")
            contentItem: QQC2.Label {
                text: i18n("Could not export the certificate.")
            }
            standardButtons: QQC2.Dialog.Ok

            onAccepted: close();
        }
    }
}
