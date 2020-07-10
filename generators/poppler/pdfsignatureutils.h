/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_PDF_SIGNATUREUTILS_H_
#define _OKULAR_GENERATOR_PDF_SIGNATUREUTILS_H_

#include <poppler-form.h>

#include "core/signatureutils.h"

#include <config-okular-poppler.h>

#ifdef HAVE_POPPLER_0_73

class PopplerCertificateInfo : public Okular::CertificateInfo
{
public:
    PopplerCertificateInfo(const Poppler::CertificateInfo &info);
    ~PopplerCertificateInfo() override;

    bool isNull() const override;
    int version() const override;
    QByteArray serialNumber() const override;
    QString issuerInfo(EntityInfoKey) const override;
    QString subjectInfo(EntityInfoKey) const override;
    QDateTime validityStart() const override;
    QDateTime validityEnd() const override;
    KeyUsageExtensions keyUsageExtensions() const override;
    QByteArray publicKey() const override;
    PublicKeyType publicKeyType() const override;
    int publicKeyStrength() const override;
    bool isSelfSigned() const override;
    QByteArray certificateData() const override;

private:
    Poppler::CertificateInfo m_info;
};

#endif

class PopplerSignatureInfo : public Okular::SignatureInfo
{
public:
    PopplerSignatureInfo(const Poppler::SignatureValidationInfo &info);
    ~PopplerSignatureInfo() override;

    SignatureStatus signatureStatus() const override;
    CertificateStatus certificateStatus() const override;
    QString signerName() const override;
    QString signerSubjectDN() const override;
    QString location() const override;
    QString reason() const override;
    HashAlgorithm hashAlgorithm() const override;
    QDateTime signingTime() const override;
    QByteArray signature() const override;
    QList<qint64> signedRangeBounds() const override;
    bool signsTotalDocument() const override;
    const Okular::CertificateInfo &certificateInfo() const override;

private:
    Poppler::SignatureValidationInfo m_info;
    Okular::CertificateInfo *m_certfiticateInfo;
};

#endif
