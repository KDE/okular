/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_GENERATOR_PDF_SIGNATUREUTILS_H_
#define _OKULAR_GENERATOR_PDF_SIGNATUREUTILS_H_

#include <poppler-form.h>
#include <poppler-version.h>

#include "core/signatureutils.h"
#include <stack>

Okular::SignatureInfo fromPoppler(const Poppler::SignatureValidationInfo &info);
Okular::CertificateInfo fromPoppler(const Poppler::CertificateInfo &info);
Okular::SignatureInfo::CertificateStatus fromPoppler(Poppler::SignatureValidationInfo::CertificateStatus status);

class PopplerCertificateStore : public Okular::CertificateStore
{
public:
    ~PopplerCertificateStore() override;

    QList<Okular::CertificateInfo> signingCertificates(bool *userCancelled) const override;
};

class SignatureSettings
{
public:
    class AlternativeLifetime
    {
    public:
        ~AlternativeLifetime();
        explicit AlternativeLifetime(SignatureSettings *settings);
        AlternativeLifetime &operator=(const AlternativeLifetime &) = delete;
        AlternativeLifetime &operator=(AlternativeLifetime &&) = delete;
        AlternativeLifetime(const AlternativeLifetime &) = delete;
        AlternativeLifetime(AlternativeLifetime &&) = delete;

    private:
        SignatureSettings *settings;
    };
    static std::shared_ptr<SignatureSettings> ref();
    AlternativeLifetime setAlternative(const std::function<char *(const char *)> &callback);
    SignatureSettings();
    ~SignatureSettings();

private:
    friend class AlternativeLifetime;
    std::stack<std::function<char *(const char *)>> alternativeStack;
    void pop();
    bool hasNSS;
};

#endif
