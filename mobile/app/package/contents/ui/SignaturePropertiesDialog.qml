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

Kirigami.OverlaySheet
{
    id: root

    property alias signatureValidityText: signatureValidity.text
    property alias documentModificationsText: documentModifications.text
    property alias signerNameText: signerName.text
    property alias signingTimeText: signingTime.text
    property alias signingLocationText: signingLocation.text
    property alias signingReasonText: signingReason.text

    property var certificateModel
    property int signatureRevisionIndex: -1

    signal saveSignatureSignedVersion(url path)

    title: i18n("Signature Properties")

    function showErrorDialog() {
        errorDialog.open();
    }

    ColumnLayout {
        // Without this the width is unreasonably narrow, potentially
        // https://invent.kde.org/frameworks/kirigami/-/merge_requests/487 fixes it
        // check when a kirigami with that is required as minimum version
        Layout.preferredWidth: Math.min(Window.window.width, 360)

        QQC2.GroupBox {
            Layout.fillWidth: true
            title: i18n("Validity Status")

            Kirigami.FormLayout {
                width: parent.width
                QQC2.Label {
                    id: signatureValidity
                    Kirigami.FormData.label: i18n("Signature Validity:")
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    id: documentModifications
                    Kirigami.FormData.label: i18n("Document Modifications:")
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }
        }
        QQC2.GroupBox {
            title: i18n("Additional Information")
            Layout.fillWidth: true

            Kirigami.FormLayout {
                id: additionalInformationLayout

                width: parent.width
                QQC2.Label {
                    id: signerName
                    Kirigami.FormData.label: i18n("Signed By:")
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    id: signingTime
                    Kirigami.FormData.label: i18n("Signing Time:")
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    id: signingReason
                    Kirigami.FormData.label: i18n("Reason:")
                    visible: text
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                QQC2.Label {
                    id: signingLocation
                    Kirigami.FormData.label: i18n("Location:")
                    visible: text
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }
        }

        QQC2.GroupBox {
            title: i18n("Document Version")
            Layout.fillWidth: true

            visible: root.signatureRevisionIndex >= 0

            RowLayout {
                width: parent.width

                QQC2.Label {
                    Layout.fillWidth: true
                    text: i18nc("Document Revision <current> of <total>", "Document Revision %1 of %2", root.signatureRevisionIndex + 1, documentItem.signaturesModel.count)
                    wrapMode: Text.Wrap
                }
                QQC2.Button {
                    text: i18n("Save Signed Version...")
                    onClicked: {
                        fileDialog.open();
                    }
                }
            }
        }

        QQC2.DialogButtonBox {
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.fillWidth: true

            QQC2.Button {
                QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.ActionRole
                text: i18n("View Certificate...")
                onClicked: {
                    var dialog = dialogComponent.createObject(Window.window, {
                        certificateModel: root.certificateModel
                    })
                    dialog.open()
                }

                Component {
                    id: dialogComponent
                    CertificateViewerDialog {
                        onSheetOpenChanged: if(!sheetOpen) {
                            destroy(1000)
                        }
                    }
                }
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
            folder: "file://" + userPaths.documents
            selectExisting: false
            onAccepted: {
                root.saveSignatureSignedVersion(fileDialog.fileUrl);
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
}
