/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "config-okular.h"
#include "executor_js_p.h"

#include "../debug_p.h"
#include "../document_p.h"

#include "event_p.h"
#include "js_app_p.h"
#include "js_console_p.h"
#include "js_data_p.h"
#include "js_display_p.h"
#include "js_document_p.h"
#include "js_event_p.h"
#include "js_field_p.h"
#include "js_fullscreen_p.h"
#include "js_global_p.h"
#include "js_ocg_p.h"
#include "js_spell_p.h"
#include "js_util_p.h"

#include <QDebug>
#include <QJSEngine>
#include <QStack>
#include <QThread>
#include <QTimer>

using namespace Okular;

class Okular::ExecutorJSPrivate
{
public:
    explicit ExecutorJSPrivate(DocumentPrivate *doc)
        : m_doc(doc)
    {
        initTypes();
    }
    ~ExecutorJSPrivate()
    {
        m_watchdogTimer->deleteLater();
        m_watchdogThread.quit();
        m_watchdogThread.wait();
    }

    void initTypes();

    void updateEvent();

    DocumentPrivate *m_doc;
    QJSEngine m_interpreter;

    QThread m_watchdogThread;
    QTimer *m_watchdogTimer = nullptr;

    QStack<Event *> m_events;
};

void ExecutorJSPrivate::initTypes()
{
    m_watchdogThread.start();
    m_watchdogTimer = new QTimer;
    m_watchdogTimer->setInterval(std::chrono::seconds(2)); // max 2 secs allowed
    m_watchdogTimer->setSingleShot(true);
    m_watchdogTimer->moveToThread(&m_watchdogThread);
    QObject::connect(m_watchdogTimer, &QTimer::timeout, &m_interpreter, [this]() { m_interpreter.setInterrupted(true); }, Qt::DirectConnection);

    m_interpreter.globalObject().setProperty(QStringLiteral("app"), m_interpreter.newQObject(new JSApp(m_doc, m_watchdogTimer)));
    m_interpreter.globalObject().setProperty(QStringLiteral("console"), m_interpreter.newQObject(new JSConsole));
    m_interpreter.globalObject().setProperty(QStringLiteral("Doc"), m_interpreter.newQObject(new JSDocument(m_doc)));
    m_interpreter.globalObject().setProperty(QStringLiteral("display"), m_interpreter.newQObject(new JSDisplay));
    m_interpreter.globalObject().setProperty(QStringLiteral("spell"), m_interpreter.newQObject(new JSSpell));
    m_interpreter.globalObject().setProperty(QStringLiteral("util"), m_interpreter.newQObject(new JSUtil));
    m_interpreter.globalObject().setProperty(QStringLiteral("global"), m_interpreter.newQObject(new JSGlobal));
}

void ExecutorJSPrivate::updateEvent()
{
    if (!m_events.isEmpty()) {
        Event *event = m_events.top();
        const auto eventVal = event ? m_interpreter.newQObject(new JSEvent(event)) : QJSValue(QJSValue::UndefinedValue);
        m_interpreter.globalObject().setProperty(QStringLiteral("event"), eventVal);
    } else {
        m_interpreter.globalObject().setProperty(QStringLiteral("event"), QJSValue(QJSValue::UndefinedValue));
    }
}

ExecutorJS::ExecutorJS(DocumentPrivate *doc)
    : d(new ExecutorJSPrivate(doc))
{
}

ExecutorJS::~ExecutorJS()
{
    JSField::clearCachedFields();
    JSApp::clearCachedFields();
    delete d;
}

void ExecutorJS::execute(const QString &script, Event *event)
{
    d->m_events.push(event);
    d->updateEvent();

    QMetaObject::invokeMethod(d->m_watchdogTimer, qOverload<>(&QTimer::start));
    d->m_interpreter.setInterrupted(false);
    auto result = d->m_interpreter.evaluate(script, QStringLiteral("okular.js"));
    QMetaObject::invokeMethod(d->m_watchdogTimer, qOverload<>(&QTimer::stop));

    if (result.isError()) {
        qCDebug(OkularCoreDebug) << "JS exception" << result.toString() << "(line " << result.property(QStringLiteral("lineNumber")).toInt() << ")";
    } else {
        qCDebug(OkularCoreDebug) << "result:" << result.toString();

        if (event) {
            qCDebug(OkularCoreDebug) << "Event Result:" << event->name() << event->type() << "value:" << event->value();
        }
    }
    d->m_events.pop();
    d->updateEvent();
}
