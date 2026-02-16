/*
    SPDX-FileCopyrightText: 2019 Bubli <Katarina.Behrens@cib.de>
    SPDX-FileCopyrightText: 2020 Albert Astals Cid <albert.astals.cid@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pdfsettingswidget.h"

#include "pdfsettings.h"
#include "pdfsignatureutils.h"
#include "popplerversion.h"

#include <KLocalizedString>
#include <KUrlRequester>

#include <KMessageBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>

QString PDFSettingsWidget::popplerEnumToSettingString(Poppler::CryptoSignBackend backend)
{
    switch (backend) {
    case Poppler::CryptoSignBackend::NSS:
        return QStringLiteral("NSS");
    case Poppler::CryptoSignBackend::GPG:
        return QStringLiteral("GPG");
    }
    return {};
}

static QString popplerEnumToUserString(Poppler::CryptoSignBackend backend)
{
    switch (backend) {
        // I'm not sure it makes sense to translate these
        // Should the translators ask for it, it should be quite simple.
    case Poppler::CryptoSignBackend::NSS:
        return QStringLiteral("NSS");
    case Poppler::CryptoSignBackend::GPG:
        return QStringLiteral("GnuPG (S/MIME)");
    }
    return {};
}

std::optional<Poppler::CryptoSignBackend> PDFSettingsWidget::settingStringToPopplerEnum(QStringView backend)
{
    if (backend == QStringLiteral("NSS")) {
        return Poppler::CryptoSignBackend::NSS;
    }
    if (backend == QStringLiteral("GPG")) {
        return Poppler::CryptoSignBackend::GPG;
    }
    return std::nullopt;
}

PDFSettingsWidget::PDFSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    m_pdfsw.setupUi(this);

    m_pdfsw.kcfg_OverprintPreviewEnabled->hide();
#if POPPLER_VERSION_MACRO < QT_VERSION_CHECK(25, 02, 90)
    m_pdfsw.kcfg_EnablePgp->hide();
#endif

    auto backends = Poppler::availableCryptoSignBackends();
    if (!backends.empty()) {
        // Let's try get the currently stored backend:
        auto currentBackend = settingStringToPopplerEnum(PDFSettings::self()->signatureBackend());
        if (!currentBackend) {
            currentBackend = Poppler::activeCryptoSignBackend();
        }
        if (currentBackend != Poppler::activeCryptoSignBackend() && currentBackend) {
            if (!Poppler::setActiveCryptoSignBackend(currentBackend.value())) {
                // erm. This must be a case of having either modified
                // the config file manually to something not available
                // in the poppler installed here or have reconfigured
                // their poppler to not have the previously selected one
                // available any longer.
                // Probably the safest bet is to take whatever is active
                currentBackend = Poppler::activeCryptoSignBackend();
            }
        }
        int selected = -1;
        for (auto backend : std::as_const(backends)) {
            if (backend == currentBackend) {
                selected = m_pdfsw.kcfg_SignatureBackend->count();
            }
            m_pdfsw.kcfg_SignatureBackend->addItem(popplerEnumToUserString(backend), QVariant(popplerEnumToSettingString(backend)));
        }
        m_pdfsw.kcfg_SignatureBackend->setProperty("kcfg_property", QByteArray("currentData"));

        m_pdfsw.kcfg_SignatureBackend->setCurrentIndex(selected);
        connect(m_pdfsw.kcfg_SignatureBackend, &QComboBox::currentIndexChanged, this, [this](int index) {
            auto backendEnum = settingStringToPopplerEnum(m_pdfsw.kcfg_SignatureBackend->itemData(index).toString());
            if (!backendEnum) {
                return;
            }
            Poppler::setActiveCryptoSignBackend(backendEnum.value());
            m_pdfsw.certDBGroupBox->setVisible(backendEnum == Poppler::CryptoSignBackend::NSS);
#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(25, 02, 90)
            m_pdfsw.kcfg_EnablePgp->setVisible(backendEnum == Poppler::CryptoSignBackend::GPG);
#endif
            m_certificatesAsked = false;
            update();
        });
#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(25, 02, 90)
        connect(m_pdfsw.kcfg_EnablePgp, &QAbstractButton::toggled, this, [this](bool checked) {
            bool wasAllowed = Poppler::arePgpSignaturesAllowed();
            if (!wasAllowed && checked) {
                KMessageBox::information(this,
                                         i18nc("@info Kind of a notice/warning", "These signatures only work between modern versions of Okular with the GnuPG based backend activated"),
                                         i18nc("@title:dialog", "Enable PGP Signatures"),
                                         QStringLiteral("enablePgpWarningShown"));
            }
            Poppler::setPgpSignaturesAllowed(checked);
            m_certificatesAsked = false;
            update();
        });
#endif

#if POPPLER_VERSION_MACRO >= QT_VERSION_CHECK(25, 02, 90)
        m_pdfsw.kcfg_EnablePgp->setVisible(currentBackend == Poppler::CryptoSignBackend::GPG);
#endif
        m_pdfsw.certDBGroupBox->setVisible(currentBackend == Poppler::CryptoSignBackend::NSS);

        m_pdfsw.loadSignaturesButton->hide();

        KUrlRequester *pDlg = new KUrlRequester();
        pDlg->setObjectName(QStringLiteral("kcfg_DBCertificatePath"));
        pDlg->setMode(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly);
        pDlg->setEnabled(false);
        m_pdfsw.formLayout->setWidget(1, QFormLayout::FieldRole, pDlg);

        connect(m_pdfsw.customRadioButton, &QRadioButton::toggled, pDlg, &KUrlRequester::setEnabled);

        if (!PDFSettings::useDefaultCertDB()) {
            m_pdfsw.customRadioButton->setChecked(true);
            m_pdfsw.defaultLabel->setVisible(false);
        }

        m_tree = new QTreeWidget(this);
        m_tree->setHeaderLabels({i18nc("Name of the person to whom the cerficate was issued", "Issued to"), i18n("E-mail"), i18nc("Certificate expiration date", "Expiration date")});
        m_tree->setRootIsDecorated(false);

        m_pdfsw.certificatesPlaceholder->addWidget(m_tree);

        connect(PDFSettings::self(), &PDFSettings::useDefaultDBChanged, this, &PDFSettingsWidget::warnRestartNeeded);
        connect(PDFSettings::self(), &PDFSettings::dBCertificatePathChanged, this, [this] {
            if (!PDFSettings::useDefaultCertDB()) {
                warnRestartNeeded();
            }
        });
        connect(m_pdfsw.loadSignaturesButton, &QPushButton::clicked, this, [this] {
            m_certificatesAsked = false;
            update();
        });
    } else {
        QHBoxLayout *lay = new QHBoxLayout(this);
        QLabel *l = new QLabel(i18n("You are using a Poppler library built without NSS support.\nAdding Digital Signatures isn't available for that reason"));
        l->setWordWrap(true);
        lay->addWidget(l);
    }
}

bool PDFSettingsWidget::event(QEvent *e)
{
    if (m_tree && e->type() == QEvent::Paint && !m_certificatesAsked) {
        m_certificatesAsked = true;
        m_tree->clear();

        // Calling st.signingCertificates(&userCancelled) from the paint event handler results
        // in "QWidget::repaint: Recursive repaint detected" warning and a crash when the
        // certificate password dialog is closed. Delay the calling to avoid it.
        auto loadCertificatesDelayed = [this]() {
            PopplerCertificateStore st;
            bool userCancelled;
            const QList<Okular::CertificateInfo> certs = st.signingCertificates(&userCancelled);

            m_pdfsw.loadSignaturesButton->setVisible(userCancelled);

            for (const auto &cert : certs) {
                QDateTime end = cert.validityEnd();
                QString validityString = end.isValid() ? end.toString(QStringLiteral("yyyy-MM-dd")) : i18nc("certificate end validity", "forever");
                new QTreeWidgetItem(m_tree,
                                    {cert.subjectInfo(Okular::CertificateInfo::EntityInfoKey::CommonName, Okular::CertificateInfo::EmptyString::TranslatedNotAvailable),
                                     cert.subjectInfo(Okular::CertificateInfo::EntityInfoKey::EmailAddress, Okular::CertificateInfo::EmptyString::TranslatedNotAvailable),
                                     validityString});
            }

            m_pdfsw.defaultLabel->setText(Poppler::getNSSDir());

            m_tree->resizeColumnToContents(1);
            m_tree->resizeColumnToContents(0);
        };
        QMetaObject::invokeMethod(this, loadCertificatesDelayed, Qt::QueuedConnection);
    }
    return QWidget::event(e);
}

void PDFSettingsWidget::warnRestartNeeded()
{
    if (!m_warnedAboutRestart) {
        if (PDFSettings::self()->signatureBackend() != QStringLiteral("NSS")) {
            return;
        }
        m_warnedAboutRestart = true;
        QMessageBox::information(this, i18n("Restart needed"), i18n("You need to restart Okular after changing the NSS directory settings"));
    }
}
