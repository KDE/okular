/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_GENERATOR_PDF_SIGNATUREUTILS_H_
#define _OKULAR_GENERATOR_PDF_SIGNATUREUTILS_H_

#include <poppler-form.h>
#include <poppler-version.h>

#include "core/signatureutils.h"

Okular::SignatureInfo fromPoppler(const Poppler::SignatureValidationInfo &info);
Okular::CertificateInfo fromPoppler(const Poppler::CertificateInfo &info);
Okular::SignatureInfo::CertificateStatus fromPoppler(Poppler::SignatureValidationInfo::CertificateStatus status);

class PopplerCertificateStore : public Okular::CertificateStore
{
public:
    ~PopplerCertificateStore() override;

    QList<Okular::CertificateInfo> signingCertificates(bool *userCancelled) const override;
};

#endif
