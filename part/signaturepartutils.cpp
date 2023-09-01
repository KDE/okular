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

namespace SignaturePartUtils
{
static QImage scaleAndFitCanvas(const QImage &input, const QSize expectedSize)
{
    if (input.size() == expectedSize) {
        return input;
    }
    const auto scaled = input.scaled(expectedSize, Qt::KeepAspectRatio);
    if (scaled.size() == expectedSize) {
        return scaled;
    }
    QImage canvas(expectedSize, QImage::Format_ARGB32);
    canvas.fill(Qt::transparent);
    const auto scaledSize = scaled.size();
    QPoint topLeft((expectedSize.width() - scaledSize.width()) / 2, (expectedSize.height() - scaledSize.height()) / 2);
    QPainter painter(&canvas);
    painter.drawImage(topLeft, scaled);
    return canvas;
}

class ImageItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::paint(painter, option, QModelIndex()); // paint the background but without any text on it.
        const auto path = index.data(Qt::DisplayRole).value<QString>();

        QImageReader reader(path);
        const QSize imageSize = reader.size();
        if (!reader.size().isNull()) {
            reader.setScaledSize(imageSize.scaled(option.rect.size(), Qt::KeepAspectRatio));
        }
        const auto input = reader.read();
        if (!input.isNull()) {
            const auto scaled = scaleAndFitCanvas(input, option.rect.size());
            painter->drawImage(option.rect.topLeft(), scaled);
        }
    }
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        Q_UNUSED(index);
        QSize defaultSize(10, 10); // let's start with a square
        if (auto view = qobject_cast<QListView *>(option.styleObject)) {
            auto frameRect = view->frameRect().size();
            frameRect.setWidth((frameRect.width() - view->style()->pixelMetric(QStyle::PM_ScrollBarExtent)) / 2 - 2 * view->frameWidth() - view->spacing());
            return defaultSize.scaled(frameRect, Qt::KeepAspectRatio);
        }
        return defaultSize;
    }
};

class RecentImagesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    static constexpr char ConfigGroup[] = "Signature";
    static constexpr char ConfigKey[] = "RecentBackgrounds";
    RecentImagesModel()
    {
        for (auto &element : KSharedConfig::openConfig()->group(QLatin1String(ConfigGroup)).readEntry<QStringList>(QLatin1String(ConfigKey), QStringList())) {
            if (QFile::exists(element)) { // maybe the image has been removed from disk since last invocation
                m_storedElements.push_back(element);
            }
        }
    }
    QVariant roleFromString(const QString &data, int role) const
    {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            return data;
        default:
            return QVariant();
        }
    }
    QVariant data(const QModelIndex &index, int role) const override
    {
        Q_ASSERT(checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid));
        int row = index.row();
        if (m_selectedFromFileSystem.has_value()) {
            if (row == 0) {
                return roleFromString(*m_selectedFromFileSystem, role);
            } else {
                row--;
            }
        }
        if (row < m_storedElements.size()) {
            return roleFromString(m_storedElements.at(row), role);
        }
        return QVariant();
    }
    int rowCount(const QModelIndex &parent = {}) const override
    {
        if (parent.isValid()) {
            return 0;
        }
        return m_storedElements.size() + (m_selectedFromFileSystem.has_value() ? 1 : 0);
    }
    void setFileSystemSelection(const QString &selection)
    {
        if (m_storedElements.contains(selection)) {
            return;
        }
        if (selection.isEmpty()) {
            if (!m_selectedFromFileSystem) {
                return;
            }
            beginRemoveRows(QModelIndex(), 0, 0);
            m_selectedFromFileSystem.reset();
            endRemoveRows();
            return;
        }
        if (!QFile::exists(selection)) {
            return;
        }
        if (m_selectedFromFileSystem) {
            m_selectedFromFileSystem = selection;
            Q_EMIT dataChanged(index(0, 0), index(0, 0));
        } else {
            beginInsertRows(QModelIndex(), 0, 0);
            m_selectedFromFileSystem = selection;
            endInsertRows();
        }
    }
    void clear()
    {
        beginResetModel();
        m_selectedFromFileSystem = {};
        m_storedElements.clear();
        endResetModel();
    }
    void removeItem(const QString &text)
    {
        if (text == m_selectedFromFileSystem) {
            beginRemoveRows(QModelIndex(), 0, 0);
            m_selectedFromFileSystem.reset();
            endRemoveRows();
            return;
        }
        auto elementIndex = m_storedElements.indexOf(text);
        auto beginRemove = elementIndex;
        if (m_selectedFromFileSystem) {
            beginRemove++;
        }
        beginRemoveRows(QModelIndex(), beginRemove, beginRemove);
        m_storedElements.removeAt(elementIndex);
        endRemoveRows();
    }
    void saveBack()
    {
        QStringList elementsToStore = m_storedElements;
        if (m_selectedFromFileSystem) {
            elementsToStore.push_front(*m_selectedFromFileSystem);
        }
        while (elementsToStore.size() > 3) {
            elementsToStore.pop_back();
        }
        KSharedConfig::openConfig()->group(QString::fromUtf8(ConfigGroup)).writeEntry(ConfigKey, elementsToStore);
    }

private:
    std::optional<QString> m_selectedFromFileSystem;
    QStringList m_storedElements;
};

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
    if (items.rowCount() < 3) {
        auto rowHeight = dialog.ui->list->sizeHintForRow(0);
        dialog.ui->list->setFixedHeight(rowHeight * items.rowCount() + (items.rowCount() - 1) * dialog.ui->list->spacing() + dialog.ui->list->contentsMargins().top() + dialog.ui->list->contentsMargins().bottom());
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
            QAction *selected = QMenu::exec(actions, view->viewport()->mapToGlobal(pos), nullptr, view);
            if (selected == &currentImage) {
                recentModel->removeItem(current.data(Qt::DisplayRole).toString());
                recentModel->saveBack();
            } else if (selected == &allImages) {
                recentModel->clear();
                recentModel->saveBack();
            }
        });
    }
    auto result = dialog.exec();

    if (result == QDialog::Rejected) {
        return std::nullopt;
    }
    const auto certNicknameToUse = dialog.ui->list->selectionModel()->currentIndex().data(Qt::DisplayRole).toString();
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
    const QString localFilePathIfAny = currentFileUrl.isLocalFile() ? QFileInfo(currentFileUrl.path()).canonicalPath() + QLatin1Char('/') : QString();
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

#include "signaturepartutils.moc"
