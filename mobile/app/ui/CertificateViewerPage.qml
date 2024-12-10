/*
 *   SPDX-FileCopyrightdescription: 2022 Albert Astals Cid <aacid@kde.org>
 *   SPDX-FileCopyrightText: 2024 Carl Schwan <carl@carlschwan.eu>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtCore
import QtQuick
import QtQuick.Window
import QtQuick.Controls as QQC2
import QtQuick.Dialogs as QQD
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.okular as Okular

FormCard.FormCardPage {
    id: root

    property Okular.CertificateModel certificateModel

    title: i18n("Certificate Viewer")

    FormCard.FormHeader {
        title: i18n("Issued By")
    }

    FormCard.FormCard {
        FormCard.FormTextDelegate {
            text: i18n("Common Name:")
            description: certificateModel.propertyVisibleValue(Okular.CertificateModel.IssuerName)
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormTextDelegate {
            text: i18n("EMail:")
            description: certificateModel.propertyVisibleValue(Okular.CertificateModel.IssuerEmail)
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormTextDelegate {
            text: i18n("Organization:")
            description: certificateModel.propertyVisibleValue(Okular.CertificateModel.IssuerOrganization)
        }
    }

    FormCard.FormHeader {
        title: i18n("Issued To")
    }

    FormCard.FormCard {
        FormCard.FormTextDelegate {
            text: i18n("Common Name:")
            description: certificateModel.propertyVisibleValue(Okular.CertificateModel.SubjectName)
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormTextDelegate {
            text: i18n("EMail:")
            description: certificateModel.propertyVisibleValue(Okular.CertificateModel.SubjectEmail)
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormTextDelegate {
            text: i18n("Organization:")
            description: certificateModel.propertyVisibleValue(Okular.CertificateModel.SubjectOrganization)
        }
    }

    FormCard.FormHeader {
        title: i18n("Validity")
    }

    FormCard.FormCard {
        FormCard.FormTextDelegate {
            text: i18n("Issued On:")
            description: certificateModel.propertyVisibleValue(Okular.CertificateModel.IssuedOn)
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormTextDelegate {
            text: i18n("Expires On:")
            description: certificateModel.propertyVisibleValue(Okular.CertificateModel.ExpiresOn)
        }
    }

    FormCard.FormHeader {
        title: i18n("Fingerprints")
    }

    FormCard.FormCard {
        FormCard.FormTextDelegate {
            text: i18n("SHA-1 Fingerprint:")
            description: certificateModel.propertyVisibleValue(Okular.CertificateModel.Sha1)
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormTextDelegate {
            text: i18n("SHA-256 Fingerprint:")
            description: certificateModel.propertyVisibleValue(Okular.CertificateModel.Sha256)
        }
    }

    FormCard.FormCard {
        visible: Kirigami.Settings.isMobile

        Layout.topMargin: Kirigami.Units.gridUnit

        FormCard.FormButtonDelegate {
            icon.name: "document-export-symbolic"
            text: i18n("Export…")
            onClicked: fileDialog.open()
        }
    }

    footer: QQC2.ToolBar {
        visible: !Kirigami.Settings.isMobile
        padding: 0
        contentItem: QQC2.DialogButtonBox {
            QQC2.Button {
                QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.ActionRole
                icon.name: "document-export-symbolic"
                text: i18n("Export…")
                onClicked: fileDialog.open()
            }

            QQC2.Button {
                QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.DestructiveRole
                visible: !Kirigami.Settings.isMobile
                text: i18n("Close")
                icon.name: "dialog-close"
                onClicked: root.closeDialog()
            }
        }
    }

    QQD.FileDialog {
        id: fileDialog
        nameFilters: i18n("Certificate File (*.cer)")
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        fileMode: QQD.FileDialog.SaveFile
        onAccepted: {
            if (!certificateModel.exportCertificateTo(fileDialog.selectedFile)) {
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
