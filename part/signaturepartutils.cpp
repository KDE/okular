/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "signaturepartutils.h"

#include "core/document.h"
#include "core/form.h"
#include "core/page.h"
#include "pageview.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMimeDatabase>

#include <KLocalizedString>
#include <KMessageBox>

namespace SignaturePartUtils
{
std::unique_ptr<Okular::CertificateInfo> getCertificateAndPasswordForSigning(PageView *pageView, Okular::Document *doc, QString *password, QString *documentPassword)
{
    const Okular::CertificateStore *certStore = doc->certificateStore();
    bool userCancelled, nonDateValidCerts;
    QList<Okular::CertificateInfo *> certs = certStore->signingCertificatesForNow(&userCancelled, &nonDateValidCerts);
    if (userCancelled) {
        return nullptr;
    }

    if (certs.isEmpty()) {
        pageView->showNoSigningCertificatesDialog(nonDateValidCerts);
        return nullptr;
    }

    QStringList items;
    QHash<QString, Okular::CertificateInfo *> nickToCert;
    for (auto cert : qAsConst(certs)) {
        items.append(cert->nickName());
        nickToCert[cert->nickName()] = cert;
    }

    bool resok = false;
    const QString certNicknameToUse = QInputDialog::getItem(pageView, i18n("Select certificate to sign with"), i18n("Certificates:"), items, 0, false, &resok);

    if (!resok) {
        qDeleteAll(certs);
        return nullptr;
    }

    // I could not find any case in which i need to enter a password to use the certificate, seems that once you unlcok the firefox/NSS database
    // you don't need a password anymore, but still there's code to do that in NSS so we have code to ask for it if needed. What we do is
    // ask if the empty password is fine, if it is we don't ask the user anything, if it's not, we ask for a password
    Okular::CertificateInfo *cert = nickToCert.value(certNicknameToUse);
    bool passok = cert->checkPassword(*password);
    while (!passok) {
        const QString title = i18n("Enter password (if any) to unlock certificate: %1", certNicknameToUse);
        bool ok;
        *password = QInputDialog::getText(pageView, i18n("Enter certificate password"), title, QLineEdit::Password, QString(), &ok);
        if (ok) {
            passok = cert->checkPassword(*password);
        } else {
            passok = false;
            break;
        }
    }

    if (doc->metaData(QStringLiteral("DocumentHasPassword")).toString() == QLatin1String("yes")) {
        *documentPassword = QInputDialog::getText(pageView, i18n("Enter document password"), i18n("Enter document password"), QLineEdit::Password, QString(), &passok);
    }

    if (passok) {
        certs.removeOne(cert);
    }
    qDeleteAll(certs);

    return passok ? std::unique_ptr<Okular::CertificateInfo>(cert) : std::unique_ptr<Okular::CertificateInfo>();
}

QString getFileNameForNewSignedFile(PageView *pageView, Okular::Document *doc)
{
    QMimeDatabase db;
    const QString typeName = doc->documentInfo().get(Okular::DocumentInfo::MimeType);
    const QMimeType mimeType = db.mimeTypeForName(typeName);
    const QString mimeTypeFilter = i18nc("File type name and pattern", "%1 (%2)", mimeType.comment(), mimeType.globPatterns().join(QLatin1Char(' ')));

    const QUrl currentFileUrl = doc->currentDocument();
    const QFileInfo currentFileInfo(currentFileUrl.fileName());
    const QString localFilePathIfAny = currentFileUrl.isLocalFile() ? QFileInfo(currentFileUrl.path()).canonicalPath() + QLatin1Char('/') : QString();
    const QString newFileName =
        localFilePathIfAny + i18nc("Used when suggesting a new name for a digitally signed file. %1 is the old file name and %2 it's extension", "%1_signed.%2", currentFileInfo.baseName(), currentFileInfo.completeSuffix());

    return QFileDialog::getSaveFileName(pageView, i18n("Save Signed File As"), newFileName, mimeTypeFilter);
}

void signUnsignedSignature(const Okular::FormFieldSignature *form, PageView *pageView, Okular::Document *doc)
{
    Q_ASSERT(form && form->signatureType() == Okular::FormFieldSignature::UnsignedSignature);
    QString password, documentPassword;
    const std::unique_ptr<Okular::CertificateInfo> cert = getCertificateAndPasswordForSigning(pageView, doc, &password, &documentPassword);
    if (!cert) {
        return;
    }

    Okular::NewSignatureData data;
    data.setCertNickname(cert->nickName());
    data.setCertSubjectCommonName(cert->subjectInfo(Okular::CertificateInfo::CommonName));
    data.setPassword(password);
    data.setDocumentPassword(documentPassword);
    password.clear();
    documentPassword.clear();

    const QString newFilePath = getFileNameForNewSignedFile(pageView, doc);

    if (!newFilePath.isEmpty()) {
        const bool success = form->sign(data, newFilePath);
        if (success) {
            Q_EMIT pageView->requestOpenFile(newFilePath, form->page()->number() + 1);
        } else {
            KMessageBox::error(pageView, i18nc("%1 is a file path", "Could not sign. Invalid certificate password or could not write to '%1'", newFilePath));
        }
    }
}

}
