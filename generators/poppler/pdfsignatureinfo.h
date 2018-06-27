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

#include "core/signatureinfo.h"

class PopplerCertificateInfo : public Okular::CertificateInfo
{
    public:
        PopplerCertificateInfo(const Poppler::CertificateInfo &info);
        ~PopplerCertificateInfo();

    private:
        KeyUsages convertToOkularKeyUsages( Poppler::CertificateInfo::KeyUsages );
        PublicKeyType convertToOkularPublicKeyType( Poppler::CertificateInfo::PublicKeyType );
};

class PopplerSignatureInfo : public Okular::SignatureInfo
{
    public:
        PopplerSignatureInfo(const Poppler::SignatureValidationInfo &info);
        ~PopplerSignatureInfo();

    private:
        SignatureStatus convertToOkularSigStatus( Poppler::SignatureValidationInfo::SignatureStatus );
        CertificateStatus convertToOkularCertStatus( Poppler::SignatureValidationInfo::CertificateStatus );
        HashAlgorithm convertToOkularHashAlg( Poppler::SignatureValidationInfo::HashAlgorithm );

};

#endif
