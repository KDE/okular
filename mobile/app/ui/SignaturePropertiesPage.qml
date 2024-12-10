/*
 *   SPDX-FileCopyrightText: 2022 Albert Astals Cid <aacid@kde.org>
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

    property alias signatureValidityText: signatureValidity.description
    property alias documentModificationsText: documentModifications.description
    property alias signerNameText: signerName.description
    property alias signingTimeText: signingTime.description
    property alias signingLocationText: signingLocation.description
    property alias signingReasonText: signingReason.description

    property Okular.CertificateModel certificateModel
    property int signatureRevisionIndex: -1

    signal saveSignatureSignedVersion(url path)

    title: i18n("Signature Properties")

    function showErrorDialog() {
        errorDialog.open();
    }

    FormCard.FormHeader {
        title: i18n("Validity Status")
    }

    FormCard.FormCard {
        FormCard.FormTextDelegate {
            id: signatureValidity
            text: i18n("Signature Validity:")
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormTextDelegate {
            id: documentModifications
            text: i18n("Document Modifications:")
        }
    }

    FormCard.FormHeader {
        title: i18n("Additional Information")
    }

    FormCard.FormCard {
        FormCard.FormTextDelegate {
            id: signerName
            text: i18n("Signed By:")
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormTextDelegate {
            id: signingTime
            text: i18n("Signing Time:")
        }

        FormCard.FormDelegateSeparator {
            visible: signingReason.visible
        }

        FormCard.FormTextDelegate {
            id: signingReason
            text: i18n("Reason:")
            visible: description
        }

        FormCard.FormDelegateSeparator {
            visible: signingLocation.visible
        }

        FormCard.FormTextDelegate {
            id: signingLocation
            text: i18n("Location:")
            visible: description
        }
    }

    FormCard.FormHeader {
        title: i18n("Document Version")
        visible: root.signatureRevisionIndex >= 0
    }

    FormCard.FormCard {
        visible: root.signatureRevisionIndex >= 0

        FormCard.FormTextDelegate {
            text: i18nc("Document Revision <current> of <total>", "Document Revision %1 of %2", root.signatureRevisionIndex + 1, documentItem.signaturesModel.count)
        }

        FormCard.FormButtonDelegate {
            text: i18n("Save Signed Version…")
            onClicked: {
                fileDialog.open();
            }
        }
    }

    FormCard.FormCard {
        visible: Kirigami.Settings.isMobile

        Layout.topMargin: Kirigami.Units.gridUnit

        FormCard.FormButtonDelegate {
            icon.name: "view-certificate-symbolic"
            text: i18n("View Certificate…")
            onClicked: {
                applicationWindow().pageStack.pushDialogLayer(Qt.resolvedUrl("CertificateViewerPage.qml"), {
                    certificateModel: root.certificateModel
                });
            }
        }
    }

    footer: QQC2.ToolBar {
        visible: !Kirigami.Settings.isMobile
        padding: 0
        contentItem: QQC2.DialogButtonBox {
            QQC2.Button {
                QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.ActionRole
                text: i18n("View Certificate…")
                icon.name: "view-certificate-symbolic"
                onClicked: {
                    applicationWindow().pageStack.pushDialogLayer(Qt.resolvedUrl("CertificateViewerPage.qml"), {
                        certificateModel: root.certificateModel
                    });
                }
            }

            QQC2.Button {
                QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.DestructiveRole
                text: i18n("Close")
                icon.name: "dialog-close"
                onClicked: root.closeDialog()
            }
        }
    }

    QQD.FileDialog {
        id: fileDialog
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        fileMode: QQD.FileDialog.SaveFile
        onAccepted: {
            root.saveSignatureSignedVersion(fileDialog.selectedFile);
        }
    }

    // TODO Use Kirigami.PromptDialog when we depend on KF >= 5.89
    // this way we can probably remove that ridiculous z value
    QQC2.Dialog {
        id: errorDialog
        z: 200
        title: i18n("Error")
        contentItem: QQC2.Label {
            text: i18n("Could not save the signature.")
        }
        standardButtons: QQC2.Dialog.Ok

        onAccepted: close();
    }
}
