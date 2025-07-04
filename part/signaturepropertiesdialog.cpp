/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "signaturepropertiesdialog.h"

#include <KColumnResizer>
#include <KLocalizedString>

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QList>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

#include "core/document.h"
#include "core/form.h"

#include "certificateviewer.h"
#include "gui/signatureguiutils.h"
#include "revisionviewer.h"

#include "kleopatraintegration.h"

static QString getValidDisplayString(const QString &str)
{
    return !str.isEmpty() ? str : i18n("Not Available");
}

SignaturePropertiesDialog::SignaturePropertiesDialog(Okular::Document *doc, const Okular::FormFieldSignature *form, QWidget *parent)
    : QDialog(parent)
    , m_doc(doc)
    , m_signatureForm(form)
{
    setModal(true);
    setWindowTitle(i18n("Signature Properties"));

    auto kleo = std::make_unique<Okular::KleopatraIntegration>(doc);

    const Okular::SignatureInfo &signatureInfo = form->signatureInfo();
    const Okular::SignatureInfo::SignatureStatus signatureStatus = signatureInfo.signatureStatus();
    const QString readableSignatureStatus = SignatureGuiUtils::getReadableSignatureStatus(signatureStatus);
    const QString modificationSummary = SignatureGuiUtils::getReadableModificationSummary(signatureInfo);
    const QString signerName = getValidDisplayString(signatureInfo.signerName());
    const QString signingTime = getValidDisplayString(QLocale().toString(signatureInfo.signingTime(), QLocale::LongFormat));
    const QString signingLocation = getValidDisplayString(signatureInfo.location());
    const QString signingReason = signatureInfo.reason();

    auto signatureStatusBox = new QGroupBox(i18n("Validity Status"));
    auto signatureStatusFormLayout = new QFormLayout(signatureStatusBox);
    signatureStatusFormLayout->setLabelAlignment(Qt::AlignLeft);
    signatureStatusFormLayout->addRow(i18n("Signature Validity:"), new QLabel(readableSignatureStatus));
    signatureStatusFormLayout->addRow(i18n("Document Modifications:"), new QLabel(modificationSummary));

    // additional information
    auto extraInfoBox = new QGroupBox(i18n("Additional Information"));
    auto extraInfoFormLayout = new QFormLayout(extraInfoBox);
    extraInfoFormLayout->setLabelAlignment(Qt::AlignLeft);
    extraInfoFormLayout->addRow(i18n("Signed By:"), new QLabel(signerName));
    extraInfoFormLayout->addRow(i18n("Signing Time:"), new QLabel(signingTime));
    if (!signingReason.isEmpty()) {
        auto label = std::make_unique<QLabel>(signingReason);
        label->setWordWrap(true);
        extraInfoFormLayout->addRow(i18n("Reason:"), label.release());
    }
    extraInfoFormLayout->addRow(i18n("Location:"), new QLabel(signingLocation));

    // keep width of column 1 same
    auto resizer = new KColumnResizer(this);
    resizer->addWidgetsFromLayout(signatureStatusFormLayout->layout(), 0);
    resizer->addWidgetsFromLayout(extraInfoFormLayout->layout(), 0);

    // document revision info
    QGroupBox *revisionBox = nullptr;
    if (signatureStatus != Okular::SignatureInfo::SignatureStatusUnknown && !signatureInfo.signsTotalDocument()) {
        revisionBox = new QGroupBox(i18n("Document Version"));
        auto revisionLayout = new QHBoxLayout(revisionBox);
        const QList<const Okular::FormFieldSignature *> signatureFormFields = SignatureGuiUtils::getSignatureFormFields(m_doc);
        revisionLayout->addWidget(new QLabel(i18nc("Document Revision <current> of <total>", "Document Revision %1 of %2", signatureFormFields.indexOf(m_signatureForm) + 1, signatureFormFields.size())));
        revisionLayout->addStretch();
        auto revisionBtn = new QPushButton(i18n("View Signed Version..."));
        connect(revisionBtn, &QPushButton::clicked, this, &SignaturePropertiesDialog::viewSignedVersion);
        revisionLayout->addWidget(revisionBtn);
    }

    // button box
    auto btnBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    auto certPropBtn = new QPushButton(i18n("View Certificate..."));
    certPropBtn->setEnabled(!signatureInfo.certificateInfo().isNull());
    auto certManagerBtn = new QPushButton(i18n("View in Certificate Manager"));
    certManagerBtn->setVisible(signatureInfo.certificateInfo().backend() == Okular::CertificateInfo::Backend::Gpg);
    certManagerBtn->setEnabled(kleo->kleopatraIntegrationActive());
    if (!kleo->kleopatraIntegrationActive()) {
        certManagerBtn->setToolTip(i18n("KDE Certificate Manager (kleopatra) not found"));
    }
    btnBox->addButton(certPropBtn, QDialogButtonBox::ActionRole);
    btnBox->addButton(certManagerBtn, QDialogButtonBox::ActionRole);
    connect(btnBox, &QDialogButtonBox::rejected, this, &SignaturePropertiesDialog::reject);
    connect(certPropBtn, &QPushButton::clicked, this, &SignaturePropertiesDialog::viewCertificateProperties);
    connect(certManagerBtn, &QPushButton::clicked, this, [this, kleo = std::move(kleo)]() { kleo->executeKeySearch(m_signatureForm->signatureInfo().certificateInfo().nickName(), window()->windowHandle()); });

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(signatureStatusBox);
    mainLayout->addWidget(extraInfoBox);
    if (revisionBox) {
        mainLayout->addWidget(revisionBox);
    }
    mainLayout->addWidget(btnBox);

    resize(mainLayout->sizeHint());
}

void SignaturePropertiesDialog::viewCertificateProperties()
{
    CertificateViewer certViewer(m_signatureForm->signatureInfo().certificateInfo(), this);
    certViewer.exec();
}

void SignaturePropertiesDialog::viewSignedVersion()
{
    const QByteArray data = m_doc->requestSignedRevisionData(m_signatureForm->signatureInfo());
    RevisionViewer revViewer(data, this);
    revViewer.viewRevision();
}

#include "moc_signaturepropertiesdialog.cpp"
