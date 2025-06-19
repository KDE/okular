/*
    SPDX-FileCopyrightText: 2025 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

     SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef KLEOPATRAINTEGRATION_H
#define KLEOPATRAINTEGRATION_H

#include "okularcore_export.h"
#include <QString>
class QWindow;

namespace Okular
{
class Document;
class KleopatraIntegrationPrivate;
class KleopatraIntegration
{
public:
    explicit KleopatraIntegration(Document *doc);
    ~KleopatraIntegration();

    /*Disable copy/move because author is lazy. Could easily be implemented if needed*/
    Q_DISABLE_COPY_MOVE(KleopatraIntegration)

    bool kleopatraIntegrationActive() const;
    void executeKeySearch(const QString &keyId, QWindow *parent) const;
    void launchKleopatra(QWindow *parent) const;

private:
    const std::unique_ptr<KleopatraIntegrationPrivate> d;
};
}

#endif // KLEOPATRAINTEGRATION_H
