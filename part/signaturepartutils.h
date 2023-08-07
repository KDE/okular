/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>
    SPDX-FileCopyrightText: 2023 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SIGNATUREPARTUTILS_H
#define OKULAR_SIGNATUREPARTUTILS_H

#include <QDialog>
#include <QFileInfo>
#include <QStyledItemDelegate>
#include <memory>
#include <optional>

#include "gui/signatureguiutils.h"
#include <KLocalizedString>

class PageView;
class Ui_SelectCertificateDialog;

namespace SignaturePartUtils
{
struct SigningInformation {
    std::unique_ptr<Okular::CertificateInfo> certificate;
    QString certificatePassword;
    QString documentPassword;
    QString reason;
    QString location;
    QString backgroundImagePath;
};

enum class SigningInformationOption { None = 0x0, BackgroundImage = 0x1 };
Q_DECLARE_FLAGS(SigningInformationOptions, SigningInformationOption);
Q_DECLARE_OPERATORS_FOR_FLAGS(SigningInformationOptions);

/** Retrieves signing information for this operation
    \return nullopt on failure, otherwise information
 */
std::optional<SigningInformation> getCertificateAndPasswordForSigning(PageView *pageView, Okular::Document *doc, SigningInformationOptions opts);

QString getFileNameForNewSignedFile(PageView *pageView, Okular::Document *doc);

inline QString getSuggestedFileNameForSignedFile(const QString &orig, const QString &suffix)
{
    QFileInfo info(orig);
    QString baseName;
    if (info.suffix() == suffix) {
        // we are in a basic plain situation with foo.pdf, or maybe foo-1.2.3.pdf
        baseName = info.completeBaseName();
    } else if (auto completeBaseName = info.completeBaseName(); completeBaseName.endsWith(suffix)) {
        // This could be a case of e.g. pdf.gz; we don't really write those
        // so chop it off.
        info = QFileInfo(completeBaseName);
        baseName = info.completeBaseName();
    } else {
        // Unsure what's going on here; maybe a pdf file with a weird or no extension.
        baseName = info.completeBaseName();
    }

    return i18nc("Used when suggesting a new name for a digitally signed file. %1 is the old file name and %2 it's extension", "%1_signed.%2", baseName, suffix);
}
void signUnsignedSignature(const Okular::FormFieldSignature *form, PageView *pageView, Okular::Document *doc);

class KeyDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const final;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const final;
};

class SelectCertificateDialog : public QDialog
{
    Q_OBJECT
public:
    std::unique_ptr<Ui_SelectCertificateDialog> ui;
    ~SelectCertificateDialog() override;

    explicit SelectCertificateDialog(QWidget *parent);
};
}

#endif
