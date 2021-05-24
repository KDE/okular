/*
    SPDX-FileCopyrightText: 2002 Wilco Greven <greven@kde.org>
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _EXTENSIONS_H_
#define _EXTENSIONS_H_

#include <kparts/browserextension.h>
#include <kparts/liveconnectextension.h>

namespace Okular
{
class Part;

class BrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT

public:
    explicit BrowserExtension(Part *);

public Q_SLOTS:
    // Automatically detected by the host.
    void print();

private:
    Part *m_part;
};

class OkularLiveConnectExtension : public KParts::LiveConnectExtension
{
    Q_OBJECT

public:
    explicit OkularLiveConnectExtension(Part *parent);

    // from LiveConnectExtension
    bool get(const unsigned long objid, const QString &field, Type &type, unsigned long &retobjid, QString &value) override;
    bool put(const unsigned long objid, const QString &field, const QString &value) override;
    bool call(const unsigned long objid, const QString &func, const QStringList &args, Type &type, unsigned long &retobjid, QString &value) override;

private:
    QString eval(const QString &script);
    void postMessage(const QStringList &args);

    bool m_inEval;
    QString m_evalRes;
};

}

#endif

/* kate: replace-tabs on; indent-width 4; */
