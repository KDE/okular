/***************************************************************************
 *   Copyright (C) 2018 by Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "signatureutils.h"

using namespace Okular;

CertificateInfo::CertificateInfo()
{
}

CertificateInfo::~CertificateInfo()
{
}

Q_DECLARE_OPERATORS_FOR_FLAGS( CertificateInfo::KeyUsages )

QByteArray CertificateInfo::version() const
{
    return QByteArray();
}

QString CertificateInfo::issuerName() const
{
    return QString();
}

QString CertificateInfo::issuerDN() const
{
    return QString();
}

QByteArray CertificateInfo::serialNumber() const
{
    return QByteArray();
}

QDateTime CertificateInfo::validityStart() const
{
    return QDateTime();
}

QDateTime CertificateInfo::validityEnd() const
{
    return QDateTime();
}

CertificateInfo::KeyUsages CertificateInfo::keyUsages() const
{
    return KuNone;
}

QByteArray CertificateInfo::publicKey() const
{
    return QByteArray();
}

CertificateInfo::PublicKeyType CertificateInfo::publicKeyType() const
{
    return OtherKey;
}

int CertificateInfo::publicKeyStrength() const
{
    return -1;
}

bool CertificateInfo::isSelfSigned() const
{
    return false;
}

QByteArray CertificateInfo::certificateData() const
{
    return QByteArray();
}


SignatureInfo::SignatureInfo()
{
}

SignatureInfo::~SignatureInfo()
{
}

SignatureInfo::SignatureStatus SignatureInfo::signatureStatus() const
{
    return SignatureStatusUnknown;
}

SignatureInfo::CertificateStatus SignatureInfo::certificateStatus() const
{
    return CertificateStatusUnknown;

}

SignatureInfo::HashAlgorithm SignatureInfo::hashAlgorithm() const
{
    return HashAlgorithmUnknown;
}

QString SignatureInfo::subjectName() const
{
    return QString();
}

QString SignatureInfo::subjectDN() const
{
    return QString();
}

QDateTime SignatureInfo::signingTime() const
{
    return QDateTime();
}

QByteArray SignatureInfo::signature() const
{
    return QByteArray();
}

QList<qint64> SignatureInfo::signedRangeBounds() const
{
    return QList<qint64>();
}

bool SignatureInfo::signsTotalDocument() const
{
    return false;
}

QString SignatureInfo::location() const
{
    return QString();
}

QString SignatureInfo::reason() const
{
    return QString();
}

CertificateInfo *SignatureInfo::certificateInfo() const
{
    return nullptr;
}
