/*
    SPDX-FileCopyrightText: 2002 Wilco Greven <greven@kde.org>
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "extensions.h"

// local includes
#include "part.h"

namespace Okular
{
/*
 * BrowserExtension class
 */
BrowserExtension::BrowserExtension(Part *parent)
    : KParts::BrowserExtension(parent)
    , m_part(parent)
{
    emit enableAction("print", true);
    setURLDropHandlingEnabled(true);
}

void BrowserExtension::print()
{
    m_part->slotPrint();
}

/*
 * OkularLiveConnectExtension class
 */
#define OKULAR_EVAL_RES_OBJ_NAME "__okular_object"
#define OKULAR_EVAL_RES_OBJ "this." OKULAR_EVAL_RES_OBJ_NAME

OkularLiveConnectExtension::OkularLiveConnectExtension(Part *parent)
    : KParts::LiveConnectExtension(parent)
    , m_inEval(false)
{
}

bool OkularLiveConnectExtension::get(const unsigned long objid, const QString &field, KParts::LiveConnectExtension::Type &type, unsigned long &retobjid, QString &value)
{
    Q_UNUSED(value)
    retobjid = objid;
    bool result = false;
    if (field == QLatin1String("postMessage")) {
        type = KParts::LiveConnectExtension::TypeFunction;
        result = true;
    }
    return result;
}

bool OkularLiveConnectExtension::put(const unsigned long objid, const QString &field, const QString &value)
{
    Q_UNUSED(objid)
    if (m_inEval) {
        if (field == QLatin1String(OKULAR_EVAL_RES_OBJ_NAME))
            m_evalRes = value;
        return true;
    }

    return false;
}

bool OkularLiveConnectExtension::call(const unsigned long objid, const QString &func, const QStringList &args, KParts::LiveConnectExtension::Type &type, unsigned long &retobjid, QString &value)
{
    retobjid = objid;
    bool result = false;
    if (func == QLatin1String("postMessage")) {
        type = KParts::LiveConnectExtension::TypeVoid;
        postMessage(args);
        value = QString();
        result = true;
    }
    return result;
}

QString OkularLiveConnectExtension::eval(const QString &script)
{
    KParts::LiveConnectExtension::ArgList args;
    args.append(qMakePair(KParts::LiveConnectExtension::TypeString, script));
    m_evalRes.clear();
    m_inEval = true;
    emit partEvent(0, QStringLiteral("eval"), args);
    m_inEval = false;
    return m_evalRes;
}

void OkularLiveConnectExtension::postMessage(const QStringList &args)
{
    QStringList arrayargs;
    for (const QString &arg : args) {
        QString newarg = arg;
        newarg.replace(QLatin1Char('\''), QLatin1String("\\'"));
        arrayargs.append(QLatin1Char('"') + newarg + QLatin1Char('"'));
    }
    const QString arrayarg = QLatin1Char('[') + arrayargs.join(QStringLiteral(", ")) + QLatin1Char(']');
    eval(QStringLiteral("if (this.messageHandler && typeof this.messageHandler.onMessage == 'function') "
                        "{ this.messageHandler.onMessage(") +
         arrayarg + QStringLiteral(") }"));
}

}

#include "moc_extensions.cpp"

/* kate: replace-tabs on; indent-width 4; */
