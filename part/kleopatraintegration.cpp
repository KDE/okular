/*
    SPDX-FileCopyrightText: 2025 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

     SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "kleopatraintegration.h"
#include "core/document_p.h"
#include <QCoreApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QWindow>

class Okular::KleopatraIntegrationPrivate
{
public:
    QString m_kleopatraPath;
    bool m_gpgActive;
    explicit KleopatraIntegrationPrivate(Okular::DocumentPrivate *docp)
    {
        m_gpgActive = (docp->m_generator->activeCertificateBackend() == Okular::CertificateInfo::Backend::Gpg);

#ifdef Q_OS_WIN
        m_kleopatraPath = QStandardPaths::findExecutable(QStringLiteral("kleopatra.exe"), {QCoreApplication::applicationDirPath()});
        if (m_kleopatraPath.isEmpty()) {
            m_kleopatraPath = QStandardPaths::findExecutable(QStringLiteral("kleopatra.exe"));
        }
#else
        m_kleopatraPath = QStandardPaths::findExecutable(QStringLiteral("kleopatra"));
#endif
    }
};

Okular::KleopatraIntegration::KleopatraIntegration(Okular::Document *doc)
    : d(std::make_unique<KleopatraIntegrationPrivate>(doc->d))
{
}

void Okular::KleopatraIntegration::executeKeySearch(const QString &keyId, QWindow *parent) const
{
    QStringList args;
    if (parent) {
        args << QStringLiteral("--parent-windowid") << QString::number(static_cast<qlonglong>(parent->winId()));
    }
    args << QStringLiteral("--query") << keyId;
    QProcess::startDetached(d->m_kleopatraPath, args);
}

void Okular::KleopatraIntegration::launchKleopatra(QWindow *parent) const
{
    QStringList args;
    if (parent) {
        args << QStringLiteral("--parent-windowid") << QString::number(static_cast<qlonglong>(parent->winId()));
    }
    QProcess::startDetached(d->m_kleopatraPath, args);
}

Okular::KleopatraIntegration::~KleopatraIntegration() = default;

bool Okular::KleopatraIntegration::kleopatraIntegrationActive() const
{
    return d->m_gpgActive && !d->m_kleopatraPath.isEmpty();
}
