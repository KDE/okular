/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>
    SPDX-FileCopyrightText: 2023 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "signaturepartutils.h"

#include "signaturepartutilsimageitemdelegate.h"
#include "signaturepartutilskconfig.h"
#include "signaturepartutilskeydelegate.h"
#include "signaturepartutilsmodel.h"
#include "signaturepartutilsrecentimagesmodel.h"
#include "signingcertificatelistmodel.h"

#include "core/document.h"
#include "core/form.h"
#include "core/page.h"
#include "pageview.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QInputDialog>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QMimeDatabase>
#include <QPainter>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include "ui_selectcertificatedialog.h"
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
namespace
{
}

namespace SignaturePartUtils
{

std::optional<SigningInformation> getCertificateAndPasswordForSigning(PageView *pageView, Okular::Document *doc, SigningInformationOptions opts)
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

    SigningCertificateListModel certificateModelUnderlying(certs);
    FilterSigningCertificateTypeListModel certificateModel;
    certificateModel.setSourceModel(&certificateModelUnderlying);
    bool showIcons = certificateModelUnderlying.hasIcons();
    int minWidth = certificateModelUnderlying.minWidth();
    auto config = KSharedConfig::openConfig();
    const QString lastNick = config->group(ConfigGroup()).readEntry<QString>(ConfigLastKeyNick(), QString());

    SelectCertificateDialog dialog(pageView);
    auto keyDelegate = new KeyDelegate(dialog.ui->list);
    keyDelegate->showIcon = showIcons;
    dialog.ui->list->setItemDelegate(keyDelegate);
    QFontMetrics fm = dialog.fontMetrics();
    dialog.ui->list->setMinimumWidth(fm.averageCharWidth() * (minWidth + 5));
    dialog.ui->list->setModel(&certificateModel);
    dialog.ui->list->setCurrentIndex(certificateModel.mapFromSource(certificateModelUnderlying.indexForNick(lastNick)));
    if (certificateModelUnderlying.types() == SignaturePartUtils::CertificateType::SMime) {
        // Only one type of certificates, no need to show filters
        for (int i = 0; i < dialog.ui->toggleTypes->count(); i++) {
            auto item = dialog.ui->toggleTypes->itemAt(i);
            if (auto w = item->widget()) {
                w->hide();
            }
        }
    } else {
        if (!certificateModelUnderlying.types().testFlag(SignaturePartUtils::CertificateType::QES)) {
            dialog.ui->qesCertificates->hide();
        }
        if (!certificateModelUnderlying.types().testFlag(SignaturePartUtils::CertificateType::PGP)) {
            dialog.ui->pgpOption->hide();
        }
    }
    QObject::connect(dialog.ui->qesCertificates, &QAbstractButton::clicked, &dialog, [&certificateModel](bool checked) {
        if (checked) {
            certificateModel.setAllowedTypes(SignaturePartUtils::CertificateType::QES);
        }
    });
    QObject::connect(dialog.ui->pgpOption, &QAbstractButton::clicked, &dialog, [&certificateModel](bool checked) {
        if (checked) {
            certificateModel.setAllowedTypes(SignaturePartUtils::CertificateType::PGP);
        }
    });
    QObject::connect(dialog.ui->allCertificateOption, &QAbstractButton::clicked, &dialog, [&certificateModel](bool checked) {
        if (checked) {
            certificateModel.setAllowedTypes(SignaturePartUtils::CertificateType::None);
        }
    });
    if (certificateModel.rowCount() < 3) {
        auto rowHeight = dialog.ui->list->sizeHintForRow(0);
        dialog.ui->list->setFixedHeight(rowHeight * certificateModel.rowCount() + (certificateModel.rowCount() - 1) * dialog.ui->list->spacing() + dialog.ui->list->contentsMargins().top() + dialog.ui->list->contentsMargins().bottom());
    }
    QObject::connect(dialog.ui->list->selectionModel(), &QItemSelectionModel::selectionChanged, &dialog, [dialog = &dialog](auto &&, auto &&) {
        // One can ctrl-click on the selected item to deselect it, that would
        // leave the selection empty, so better prevent the OK button
        // from being usable
        dialog->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(dialog->ui->list->selectionModel()->hasSelection());
    });

    RecentImagesModel imagesModel;
    if (!(opts & SigningInformationOption::BackgroundImage)) {
        dialog.ui->backgroundInput->hide();
        dialog.ui->backgroundLabel->hide();
        dialog.ui->recentBackgrounds->hide();
        dialog.ui->recentLabel->hide();
        dialog.ui->backgroundButton->hide();
    } else {
        dialog.ui->recentBackgrounds->setModel(&imagesModel);
        dialog.ui->recentBackgrounds->setSelectionMode(QAbstractItemView::SingleSelection);
        dialog.ui->recentBackgrounds->setItemDelegate(new ImageItemDelegate);
        dialog.ui->recentBackgrounds->setViewMode(QListView::IconMode);
        dialog.ui->recentBackgrounds->setDragEnabled(false);
        dialog.ui->recentBackgrounds->setSpacing(3);
        dialog.ui->recentBackgrounds->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(dialog.ui->recentBackgrounds, &QListView::activated, &dialog, [&lineEdit = dialog.ui->backgroundInput](const QModelIndex &idx) { lineEdit->setText(idx.data(Qt::DisplayRole).toString()); });
        const bool haveRecent = imagesModel.rowCount(QModelIndex()) != 0;
        if (!haveRecent) {
            dialog.ui->recentBackgrounds->hide();
            dialog.ui->recentLabel->hide();
            QObject::connect(&imagesModel, &QAbstractItemModel::rowsInserted, &dialog, [&dialog]() {
                dialog.ui->recentBackgrounds->show();
                dialog.ui->recentLabel->show();
            });
        }

        QObject::connect(dialog.ui->backgroundInput, &QLineEdit::textChanged, &dialog, [recentModel = &imagesModel, selectionModel = dialog.ui->recentBackgrounds->selectionModel()](const QString &newText) {
            recentModel->setFileSystemSelection(newText);
            /*Update selection*/
            for (int row = 0; row < recentModel->rowCount(); row++) {
                const auto index = recentModel->index(row, 0);
                if (index.data().toString() == newText) {
                    selectionModel->select(index, QItemSelectionModel::ClearAndSelect);
                    break;
                }
            }
        });
        // needs to happen after textChanged connection on backgroundInput
        if (haveRecent) {
            dialog.ui->backgroundInput->setText(imagesModel.index(0, 0).data().toString());
        }

        QObject::connect(dialog.ui->backgroundButton, &QPushButton::clicked, &dialog, [lineEdit = dialog.ui->backgroundInput]() {
            const auto supportedFormats = QImageReader::supportedImageFormats();
            QString formats;
            for (const auto &format : supportedFormats) {
                if (!formats.isEmpty()) {
                    formats += QLatin1Char(' ');
                }
                formats += QStringLiteral("*.") + QString::fromUtf8(format);
            }
            const QString imageFormats = i18nc("file types in a file open dialog", "Images (%1)", formats);
            const QString filename = QFileDialog::getOpenFileName(lineEdit, i18n("Select background image"), QDir::homePath(), imageFormats);
            lineEdit->setText(filename);
        });
        QObject::connect(dialog.ui->recentBackgrounds, &QWidget::customContextMenuRequested, &dialog, [recentModel = &imagesModel, view = dialog.ui->recentBackgrounds](QPoint pos) {
            auto current = view->indexAt(pos);
            QAction currentImage(i18n("Forget image"));
            QAction allImages(i18n("Forget all images"));
            QList<QAction *> actions;
            if (current.isValid()) {
                actions.append(&currentImage);
            }
            if (recentModel->rowCount() > 1 || actions.empty()) {
                actions.append(&allImages);
            }
            const QAction *selected = QMenu::exec(actions, view->viewport()->mapToGlobal(pos), nullptr, view);
            if (selected == &currentImage) {
                recentModel->removeItem(current.data(Qt::DisplayRole).toString());
                recentModel->saveBack();
            } else if (selected == &allImages) {
                recentModel->clear();
                recentModel->saveBack();
            }
        });
    }
    dialog.ui->reasonInput->setText(config->group(ConfigGroup()).readEntry(ConfigLastReason(), QString()));
    dialog.ui->locationInput->setText(config->group(ConfigGroup()).readEntry(ConfigLastLocation(), QString()));
    auto result = dialog.exec();

    if (result == QDialog::Rejected) {
        return std::nullopt;
    }
    auto cert = dialog.ui->list->currentIndex().data(CertRole).value<Okular::CertificateInfo>();
    auto backGroundImage = dialog.ui->backgroundInput->text();
    if (!backGroundImage.isEmpty()) {
        if (QFile::exists(backGroundImage)) {
            imagesModel.setFileSystemSelection(backGroundImage);
            imagesModel.saveBack();
        } else {
            // no need to send a nonworking image anywhere
            backGroundImage.clear();
        }
    }

    // I could not find any case in which i need to enter a password to use the certificate, seems that once you unlcok the firefox/NSS database
    // you don't need a password anymore, but still there's code to do that in NSS so we have code to ask for it if needed. What we do is
    // ask if the empty password is fine, if it is we don't ask the user anything, if it's not, we ask for a password
    bool passok = cert.checkPassword(password);
    while (!passok) {
        const QString title = i18n("Enter password (if any) to unlock certificate: %1", cert.nickName());
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
        config->group(ConfigGroup()).writeEntry(ConfigLastKeyNick(), cert.nickName());
        config->group(ConfigGroup()).writeEntry(ConfigLastReason(), dialog.ui->reasonInput->text());
        config->group(ConfigGroup()).writeEntry(ConfigLastLocation(), dialog.ui->locationInput->text());
        return SigningInformation {std::make_unique<Okular::CertificateInfo>(std::move(cert)), password, documentPassword, dialog.ui->reasonInput->text(), dialog.ui->locationInput->text(), backGroundImage};
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
    const QString localFilePathIfAny = currentFileUrl.isLocalFile() ? QFileInfo(currentFileUrl.path()).canonicalPath() + QLatin1Char('/') : QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString newFileName = localFilePathIfAny + getSuggestedFileNameForSignedFile(currentFileUrl.fileName(), mimeType.preferredSuffix());

    return QFileDialog::getSaveFileName(pageView, i18n("Save Signed File As"), newFileName, mimeTypeFilter);
}

void signUnsignedSignature(const Okular::FormFieldSignature *form, PageView *pageView, Okular::Document *doc)
{
    Q_ASSERT(form && form->signatureType() == Okular::FormFieldSignature::UnsignedSignature);
    const std::optional<SigningInformation> signingInfo = getCertificateAndPasswordForSigning(pageView, doc, SigningInformationOption::None);
    if (!signingInfo) {
        return;
    }

    Okular::NewSignatureData data;
    data.setCertNickname(signingInfo->certificate->nickName());
    data.setCertSubjectCommonName(signingInfo->certificate->subjectInfo(Okular::CertificateInfo::CommonName, Okular::CertificateInfo::EmptyString::TranslatedNotAvailable));
    data.setPassword(signingInfo->certificatePassword);
    data.setDocumentPassword(signingInfo->documentPassword);
    data.setReason(signingInfo->reason);
    data.setLocation(signingInfo->location);

    const QString newFilePath = getFileNameForNewSignedFile(pageView, doc);

    if (!newFilePath.isEmpty()) {
        const std::pair<Okular::SigningResult, QString> success = form->sign(data, newFilePath);
        switch (success.first) {
        case Okular::SigningSuccess: {
            Q_EMIT pageView->requestOpenNewlySignedFile(newFilePath, form->page()->number() + 1);
            break;
        }
        case Okular::FieldAlreadySigned: // We should not end up here
        case Okular::KeyMissing:         // unless the user modified the key store after opening the dialog, this should not happen
        case Okular::InternalSigningError:
            KMessageBox::detailedError(pageView, errorString(success.first, static_cast<int>(success.first)), success.second);
            break;
        case Okular::GenericSigningError:
            KMessageBox::detailedError(pageView, errorString(success.first, newFilePath), success.second);
            break;
        case Okular::UserCancelled:
            break;
        case Okular::BadPassphrase:
            KMessageBox::detailedError(pageView, errorString(success.first, {}), success.second);
            break;
        case Okular::SignatureWriteFailed:
            KMessageBox::detailedError(pageView, errorString(success.first, newFilePath), success.second);
            break;
        }
    }
}

SelectCertificateDialog::SelectCertificateDialog(QWidget *parent)
    : QDialog(parent)
    , ui {std::make_unique<Ui_SelectCertificateDialog>()}
{
    ui->setupUi(this);
}
SelectCertificateDialog::~SelectCertificateDialog() = default;
}
