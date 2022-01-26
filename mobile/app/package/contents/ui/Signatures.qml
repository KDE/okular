/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Window 2.15
import org.kde.kirigami 2.17 as Kirigami
import org.kde.kitemmodels 1.0

QQC2.ScrollView {
    id: root

    signal dialogOpened

    ListView {
        model: KDescendantsProxyModel {
            model: documentItem.signaturesModel
            expandsByDefault: false
        }

        delegate: TreeItem {

            function displayString(str) {
                return str ? str : i18n("Not Available");
            }

            text: model.display
            onClicked: {
                if (!model.isUnsignedSignature) {
                    var dialog = dialogComponent.createObject(Window.window, {
                        signatureValidityText: model.readableStatus,
                        documentModificationsText: model.readableModificationSummary,
                        signerNameText: displayString(model.signerName),
                        signingTimeText: displayString(model.signingTime),
                        signingLocationText: model.signingLocation,
                        signingReasonText: model.signingReason,
                        certificateModel: model.certificateModel,
                        signatureRevisionIndex: model.signatureRevisionIndex
                    })
                    dialog.open()
                    root.dialogOpened();
                }
            }
        }

        Component {
            id: dialogComponent
            SignaturePropertiesDialog {
                id: dialog
                onSheetOpenChanged: if(!sheetOpen) {
                    destroy(1000)
                }
                onSaveSignatureSignedVersion: (path) => {
                    if (!documentItem.signaturesModel.saveSignedVersion(signatureRevisionIndex, path)) {
                        dialog.showErrorDialog();
                    }
                }
            }
        }
    }
}
