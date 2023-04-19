/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>
    SPDX-FileCopyrightText: 2023 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "signaturepartutils.h"

#include "core/document.h"
#include "core/form.h"
#include "core/page.h"
#include "pageview.h"

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLabel>
#include <QListView>
#include <QMimeDatabase>
#include <QPainter>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include "ui_selectcertificatedialog.h"
#include <KLocalizedString>
#include <KMessageBox>

namespace SignaturePartUtils
{

std::optional<SigningInformation> getCertificateAndPasswordForSigning(PageView *pageView, Okular::Document *doc)
{
    const Okular::CertificateStore *certStore = doc->certificateStore();
    bool userCancelled, nonDateValidCerts;
    QList<Okular::CertificateInfo> certs = certStore->signingCertificatesForNow(&userCancelled, &nonDateValidCerts);
    if (userCancelled) {
        return std::nullopt;
    }

    if (certs.isEmpty()) {
        pageView->showNoSigningCertificatesDialog(nonDateValidCerts);
        return std::nullopt;
    }
    QString password;
    QString documentPassword;

    QStandardItemModel items;
    QHash<QString, Okular::CertificateInfo> nickToCert;
    int minWidth = -1;
    for (const auto &cert : qAsConst(certs)) {
        auto item = std::make_unique<QStandardItem>();
        QString commonName = cert.subjectInfo(Okular::CertificateInfo::CommonName, Okular::CertificateInfo::EmptyString::Empty);
        item->setData(commonName, Qt::UserRole);
        QString emailAddress = cert.subjectInfo(Okular::CertificateInfo::EmailAddress, Okular::CertificateInfo::EmptyString::Empty);
        item->setData(emailAddress, Qt::UserRole + 1);

        minWidth = std::max(minWidth, std::max(cert.nickName().size(), emailAddress.size() + commonName.size()));

        item->setData(cert.nickName(), Qt::DisplayRole);
        item->setData(cert.subjectInfo(Okular::CertificateInfo::DistinguishedName, Okular::CertificateInfo::EmptyString::Empty), Qt::ToolTipRole);
        item->setEditable(false);
        items.appendRow(item.release());
        nickToCert[cert.nickName()] = cert;
    }

    SelectCertificateDialog dialog(pageView);
    QFontMetrics fm = dialog.fontMetrics();
    dialog.ui->list->setMinimumWidth(fm.averageCharWidth() * (minWidth + 5));
    dialog.ui->list->setModel(&items);
    dialog.ui->list->selectionModel()->select(items.index(0, 0), QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
    QObject::connect(dialog.ui->list->selectionModel(), &QItemSelectionModel::selectionChanged, &dialog, [dialog = &dialog](auto &&, auto &&) {
        // One can ctrl-click on the selected item to deselect it, that would
        // leave the selection empty, so better prevent the OK button
        // from being usable
        dialog->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(dialog->ui->list->selectionModel()->hasSelection());
    });
    auto result = dialog.exec();

    if (result == QDialog::Rejected) {
        return std::nullopt;
    }
    const auto certNicknameToUse = dialog.ui->list->selectionModel()->currentIndex().data(Qt::DisplayRole).toString();

    // I could not find any case in which i need to enter a password to use the certificate, seems that once you unlcok the firefox/NSS database
    // you don't need a password anymore, but still there's code to do that in NSS so we have code to ask for it if needed. What we do is
    // ask if the empty password is fine, if it is we don't ask the user anything, if it's not, we ask for a password
    Okular::CertificateInfo cert = nickToCert.value(certNicknameToUse);
    bool passok = cert.checkPassword(password);
    while (!passok) {
        const QString title = i18n("Enter password (if any) to unlock certificate: %1", certNicknameToUse);
        bool ok;
        password = QInputDialog::getText(pageView, i18n("Enter certificate password"), title, QLineEdit::Password, QString(), &ok);
        if (ok) {
            passok = cert.checkPassword(password);
        } else {
            passok = false;
            break;
        }
    }
    if (!passok) {
        return std::nullopt;
    }

    if (doc->metaData(QStringLiteral("DocumentHasPassword")).toString() == QLatin1String("yes")) {
        documentPassword = QInputDialog::getText(pageView, i18n("Enter document password"), i18n("Enter document password"), QLineEdit::Password, QString(), &passok);
    }

    if (passok) {
        return SigningInformation {std::make_unique<Okular::CertificateInfo>(std::move(cert)), password, documentPassword};
    }
    return std::nullopt;
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
    const std::optional<SigningInformation> signingInfo = getCertificateAndPasswordForSigning(pageView, doc);
    if (!signingInfo) {
        return;
    }

    Okular::NewSignatureData data;
    data.setCertNickname(signingInfo->certificate->nickName());
    data.setCertSubjectCommonName(signingInfo->certificate->subjectInfo(Okular::CertificateInfo::CommonName, Okular::CertificateInfo::EmptyString::TranslatedNotAvailable));
    data.setPassword(signingInfo->certificatePassword);
    data.setDocumentPassword(signingInfo->documentPassword);

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

SelectCertificateDialog::SelectCertificateDialog(QWidget *parent)
    : QDialog(parent)
    , ui {std::make_unique<Ui_SelectCertificateDialog>()}
{
    ui->setupUi(this);
    ui->list->setItemDelegate(new KeyDelegate(ui->list));
}
SelectCertificateDialog::~SelectCertificateDialog() = default;

QSize KeyDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto baseSize = QStyledItemDelegate::sizeHint(option, index);
    baseSize.setHeight(baseSize.height() * 2);
    return baseSize;
}

void KeyDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto style = option.widget ? option.widget->style() : QApplication::style();

    QStyledItemDelegate::paint(painter, option, QModelIndex()); // paint the background but without any text on it.

    QPalette::ColorGroup cg;
    if (option.state & QStyle::State_Active) {
        cg = QPalette::Normal;
    } else {
        cg = QPalette::Inactive;
    }

    if (option.state & QStyle::State_Selected) {
        painter->setPen(QPen {option.palette.brush(cg, QPalette::HighlightedText), 0});
    } else {
        painter->setPen(QPen {option.palette.brush(cg, QPalette::Text), 0});
    }

    auto textRect = option.rect;
    int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &option, option.widget) + 1;
    textRect.adjust(textMargin, 0, -textMargin, 0);

    QRect topHalf {textRect.x(), textRect.y(), textRect.width(), textRect.height() / 2};
    QRect bottomHalf {textRect.x(), textRect.y() + textRect.height() / 2, textRect.width(), textRect.height() / 2};

    style->drawItemText(painter, topHalf, (option.displayAlignment & Qt::AlignVertical_Mask) | Qt::AlignLeft, option.palette, true, index.data(Qt::DisplayRole).toString());
    style->drawItemText(painter, bottomHalf, (option.displayAlignment & Qt::AlignVertical_Mask) | Qt::AlignRight, option.palette, true, index.data(Qt::UserRole + 1).toString());
    style->drawItemText(painter, bottomHalf, (option.displayAlignment & Qt::AlignVertical_Mask) | Qt::AlignLeft, option.palette, true, index.data(Qt::UserRole).toString());
}
}
