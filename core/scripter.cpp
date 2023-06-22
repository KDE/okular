/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "scripter.h"

#include "config-okular.h"

#include <QDebug>
#include <QFile>

#include "debug_p.h"
#include "script/executor_js_p.h"

using namespace Okular;

class Okular::ScripterPrivate
{
public:
    explicit ScripterPrivate(DocumentPrivate *doc)
        : m_doc(doc)
#if HAVE_JS
        , m_js(nullptr)
#endif
        , m_event(nullptr)
    {
    }

    DocumentPrivate *m_doc;
#if HAVE_JS
    QScopedPointer<ExecutorJS> m_js;
#endif
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
    qCDebug(OkularCoreDebug) << "executing the script:" << script;
#if HAVE_JS
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
        if (!d->m_js) {
            d->m_js.reset(new ExecutorJS(d->m_doc));
        }
        d->m_js->execute(builtInScript + script, d->m_event);
    }
#endif
}

void Scripter::setEvent(Event *event)
{
    d->m_event = event;
}

Event *Scripter::event() const
{
    return d->m_event;
}
