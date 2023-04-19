/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SIGNATUREPARTUTILS_H
#define OKULAR_SIGNATUREPARTUTILS_H

#include <memory>
#include <optional>

#include "gui/signatureguiutils.h"

class PageView;

namespace SignaturePartUtils
{
struct SigningInformation {
    std::unique_ptr<Okular::CertificateInfo> certificate;
    QString certificatePassword;
    QString documentPassword;
};

/** Retrieves signing information for this operation
    \return nullopt on failure, otherwise information
 */
std::optional<SigningInformation> getCertificateAndPasswordForSigning(PageView *pageView, Okular::Document *doc);

QString getFileNameForNewSignedFile(PageView *pageView, Okular::Document *doc);
void signUnsignedSignature(const Okular::FormFieldSignature *form, PageView *pageView, Okular::Document *doc);

}

#endif
