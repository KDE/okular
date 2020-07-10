/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2008 by Harri Porten <porten@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "executor_kjs_p.h"

#include <kjs/kjsarguments.h>
#include <kjs/kjsinterpreter.h>
#include <kjs/kjsobject.h>
#include <kjs/kjsprototype.h>
#include <kjs_version.h>

#include <QDebug>

#include "../debug_p.h"
#include "../document_p.h"

#include "config-okular.h"
#include "event_p.h"
#include "kjs_app_p.h"
#include "kjs_console_p.h"
#include "kjs_data_p.h"
#include "kjs_display_p.h"
#include "kjs_document_p.h"
#include "kjs_event_p.h"
#include "kjs_field_p.h"
#include "kjs_fullscreen_p.h"
#include "kjs_ocg_p.h"
#include "kjs_spell_p.h"
#include "kjs_util_p.h"

using namespace Okular;

class Okular::ExecutorKJSPrivate
{
public:
    ExecutorKJSPrivate(DocumentPrivate *doc)
        : m_doc(doc)
    {
        initTypes();
    }
    ~ExecutorKJSPrivate()
    {
        delete m_interpreter;
    }

    void initTypes();

    DocumentPrivate *m_doc;
    KJSInterpreter *m_interpreter;
    KJSGlobalObject m_docObject;
};

void ExecutorKJSPrivate::initTypes()
{
    m_docObject = JSDocument::wrapDocument(m_doc);
    m_interpreter = new KJSInterpreter(m_docObject);

#if KJS_VERSION > QT_VERSION_CHECK(5, 71, 0)
    m_interpreter->setTimeoutTime(2000); // max 2 secs allowed
#endif
    KJSContext *ctx = m_interpreter->globalContext();

    JSApp::initType(ctx);
    JSFullscreen::initType(ctx);
    JSConsole::initType(ctx);
    JSData::initType(ctx);
    JSDisplay::initType(ctx);
    JSDocument::initType(ctx);
    JSEvent::initType(ctx);
    JSField::initType(ctx);
    JSOCG::initType(ctx);
    JSSpell::initType(ctx);
    JSUtil::initType(ctx);

    m_docObject.setProperty(ctx, QStringLiteral("app"), JSApp::object(ctx, m_doc));
    m_docObject.setProperty(ctx, QStringLiteral("console"), JSConsole::object(ctx));
    m_docObject.setProperty(ctx, QStringLiteral("Doc"), m_docObject);
    m_docObject.setProperty(ctx, QStringLiteral("display"), JSDisplay::object(ctx));
    m_docObject.setProperty(ctx, QStringLiteral("OCG"), JSOCG::object(ctx));
    m_docObject.setProperty(ctx, QStringLiteral("spell"), JSSpell::object(ctx));
    m_docObject.setProperty(ctx, QStringLiteral("util"), JSUtil::object(ctx));
}

ExecutorKJS::ExecutorKJS(DocumentPrivate *doc)
    : d(new ExecutorKJSPrivate(doc))
{
}

ExecutorKJS::~ExecutorKJS()
{
    JSField::clearCachedFields();
    JSApp::clearCachedFields();
    JSOCG::clearCachedFields();
    delete d;
}

void ExecutorKJS::execute(const QString &script, Event *event)
{
#if 0
    QString script2;
    QString errMsg;
    int errLine;
    if ( !KJSInterpreter::normalizeCode( script, &script2, &errLine, &errMsg ) )
    {
        qCWarning(OkularCoreDebug) << "Parse error during normalization!";
        script2 = script;
    }
#endif

    KJSContext *ctx = d->m_interpreter->globalContext();

    d->m_docObject.setProperty(ctx, QStringLiteral("event"), event ? JSEvent::wrapEvent(ctx, event) : KJSUndefined());

#if KJS_VERSION > QT_VERSION_CHECK(5, 71, 0)
    d->m_interpreter->startTimeoutCheck();
#endif
    KJSResult result = d->m_interpreter->evaluate(QStringLiteral("okular.js"), 1, script, &d->m_docObject);
#if KJS_VERSION > QT_VERSION_CHECK(5, 71, 0)
    d->m_interpreter->stopTimeoutCheck();
#endif

    if (result.isException() || ctx->hasException()) {
        qCDebug(OkularCoreDebug) << "JS exception" << result.errorMessage();
    } else {
        qCDebug(OkularCoreDebug) << "result:" << result.value().toString(ctx);

        if (event) {
            qCDebug(OkularCoreDebug) << "Event Result:" << event->name() << event->type() << "value:" << event->value();
        }
    }
}
