/*
    SPDX-FileCopyrightText: 2023-2025 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef SIGNATUREPARTUTILSMODEL_H
#define SIGNATUREPARTUTILSMODEL_H

#include <qnamespace.h>

namespace SignaturePartUtils
{

enum SignatureListRoles { NickRole = Qt::UserRole, NickDisplayRole, CommonNameRole, EmailRole, CertRole };

enum class CertificateType { None = 0x0, SMime = 0x1, QES = 0x2 };
Q_DECLARE_FLAGS(CertificateTypes, CertificateType)
}
Q_DECLARE_OPERATORS_FOR_FLAGS(SignaturePartUtils::CertificateTypes);
#endif // SIGNATUREPARTUTILSMODEL_H
