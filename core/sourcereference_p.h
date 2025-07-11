/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SOURCEREFERENCE_P_H
#define OKULAR_SOURCEREFERENCE_P_H

#include "sourcereference.h"

#include <optional>

class QUrl;

namespace Okular
{
std::optional<SourceReference> extractLilyPondSourceReference(const QUrl &url);
QString sourceReferenceToolTip(const SourceReference &sourceReference);

}

#endif
