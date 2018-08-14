/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_PDF_SIGNATUREINFO_H_
#define _OKULAR_GENERATOR_PDF_SIGNATUREINFO_H_

#include <poppler-form.h>

#include "core/signatureutils.h"

class PopplerCertificateInfo : public Okular::CertificateInfo
{
    public:
        PopplerCertificateInfo(const Poppler::CertificateInfo &info);
        ~PopplerCertificateInfo();

    private:
        KeyUsages convertToOkularKeyUsages( Poppler::CertificateInfo::KeyUsages );
        PublicKeyType convertToOkularPublicKeyType( Poppler::CertificateInfo::PublicKeyType );
};

class PopplerSignatureInfoPrivate;

class PopplerSignatureInfo : public Okular::SignatureInfo
{
    public:
        PopplerSignatureInfo( const Poppler::SignatureValidationInfo &info );
        virtual ~PopplerSignatureInfo();

        SignatureStatus signatureStatus() const override;
        CertificateStatus certificateStatus() const override;
        QString subjectName() const override;
        QString subjectDN() const override;
        HashAlgorithm hashAlgorithm() const override;
        QDateTime signingTime() const override;
        QByteArray signature() const override;
        QList<qint64> signedRangeBounds() const override;
        bool signsTotalDocument() const override;
        Okular::CertificateInfo certificateInfo() const override;

    private:
        Poppler::SignatureValidationInfo *m_info;
};

#endif
