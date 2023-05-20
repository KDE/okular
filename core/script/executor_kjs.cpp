/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "config-okular.h"
#include "executor_kjs_p.h"

#include "../debug_p.h"
#include "../document_p.h"

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

#include <QDebug>
#include <QJSEngine>
#include <QThread>
#include <QTimer>

using namespace Okular;

class Okular::ExecutorKJSPrivate
{
public:
    explicit ExecutorKJSPrivate(DocumentPrivate *doc)
        : m_doc(doc)
    {
        initTypes();
    }
    ~ExecutorKJSPrivate()
    {
        m_watchdogTimer->deleteLater();
        m_watchdogThread.quit();
        m_watchdogThread.wait();
    }

    void initTypes();

    DocumentPrivate *m_doc;
    QJSEngine m_interpreter;

    QThread m_watchdogThread;
    QTimer *m_watchdogTimer = nullptr;
};

void ExecutorKJSPrivate::initTypes()
{
    m_watchdogThread.start();
    m_watchdogTimer = new QTimer;
    m_watchdogTimer->setInterval(std::chrono::seconds(2)); // max 2 secs allowed
    m_watchdogTimer->setSingleShot(true);
    m_watchdogTimer->moveToThread(&m_watchdogThread);
    QObject::connect(
        m_watchdogTimer, &QTimer::timeout, &m_interpreter, [this]() { m_interpreter.setInterrupted(true); }, Qt::DirectConnection);

    m_interpreter.globalObject().setProperty(QStringLiteral("app"), m_interpreter.newQObject(new JSApp(m_doc, m_watchdogTimer)));
    m_interpreter.globalObject().setProperty(QStringLiteral("console"), m_interpreter.newQObject(new JSConsole));
    m_interpreter.globalObject().setProperty(QStringLiteral("Doc"), m_interpreter.newQObject(new JSDocument(m_doc)));
    m_interpreter.globalObject().setProperty(QStringLiteral("display"), m_interpreter.newQObject(new JSDisplay));
    m_interpreter.globalObject().setProperty(QStringLiteral("spell"), m_interpreter.newQObject(new JSSpell));
    m_interpreter.globalObject().setProperty(QStringLiteral("util"), m_interpreter.newQObject(new JSUtil));
}

ExecutorKJS::ExecutorKJS(DocumentPrivate *doc)
    : d(new ExecutorKJSPrivate(doc))
{
}

ExecutorKJS::~ExecutorKJS()
{
    JSField::clearCachedFields();
    JSApp::clearCachedFields();
    delete d;
}

void ExecutorKJS::execute(const QString &script, Event *event)
{
    const auto eventVal = event ? d->m_interpreter.newQObject(new JSEvent(event)) : QJSValue(QJSValue::UndefinedValue);
    d->m_interpreter.globalObject().setProperty(QStringLiteral("event"), eventVal);

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
}
