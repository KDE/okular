/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "scripter.h"

#include <QDebug>
#include <QFile>

#include "debug_p.h"
#include "script/executor_kjs_p.h"

using namespace Okular;

class Okular::ScripterPrivate
{
public:
    ScripterPrivate(DocumentPrivate *doc)
        : m_doc(doc)
        , m_kjs(nullptr)
        , m_event(nullptr)
    {
    }

    DocumentPrivate *m_doc;
    QScopedPointer<ExecutorKJS> m_kjs;
    Event *m_event;
};

Scripter::Scripter(DocumentPrivate *doc)
    : d(new ScripterPrivate(doc))
{
}

Scripter::~Scripter()
{
    delete d;
}

void Scripter::execute(ScriptType type, const QString &script)
{
    qCDebug(OkularCoreDebug) << "executing the script:";
#if 0
    if ( script.length() < 1000 )
        qDebug() << script;
    else
        qDebug() << script.left( 1000 ) << "[...]";
#endif
    static QString builtInScript;
    if (builtInScript.isNull()) {
        QFile builtInResource(QStringLiteral(":/script/builtin.js"));
        if (!builtInResource.open(QIODevice::ReadOnly)) {
            qCDebug(OkularCoreDebug) << "failed to load builtin script";
        } else {
            builtInScript = QString::fromUtf8(builtInResource.readAll());
            builtInResource.close();
        }
    }

    switch (type) {
    case JavaScript:
        if (!d->m_kjs) {
            d->m_kjs.reset(new ExecutorKJS(d->m_doc));
        }
        d->m_kjs->execute(builtInScript + script, d->m_event);
    }
}

void Scripter::setEvent(Event *event)
{
    d->m_event = event;
}

Event *Scripter::event() const
{
    return d->m_event;
}
