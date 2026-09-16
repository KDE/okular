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
#include <QThread>
#include <QTimer>
#include <stack>

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
    std::unique_ptr<JSApp> m_jsapp;
    std::unique_ptr<JSDocument> m_jsdocument;
    std::unique_ptr<JSDisplay> m_jsdisplay;
    std::unique_ptr<JSSpell> m_jsspell;
    std::unique_ptr<JSUtil> m_jsutil;
    std::unique_ptr<JSGlobal> m_jsglobal;
    std::shared_ptr<JSFieldCache> m_fieldCache = std::make_shared<JSFieldCache>();
    QJSEngine m_interpreter;

    QThread m_watchdogThread;
    QTimer *m_watchdogTimer = nullptr;

    std::stack<std::pair<std::shared_ptr<Event>, std::unique_ptr<JSEvent>>> m_events;

    void installGlobalCppProperty(const QString &name, QObject *value)
    {
        auto jsValue = m_interpreter.newQObject(value);
        QJSEngine::setObjectOwnership(value, QJSEngine::CppOwnership);
        m_interpreter.globalObject().setProperty(name, jsValue);
    }
};

static std::pair<std::shared_ptr<Event>, std::unique_ptr<JSEvent>> wrapEvent(const std::shared_ptr<Event> &event, const std::shared_ptr<JSFieldCache> &cache)
{
    if (event) {
        return std::make_pair(event, std::make_unique<JSEvent>(event, cache));
    }
    return std::make_pair(event, std::unique_ptr<JSEvent>());
}

void ExecutorJSPrivate::initTypes()
{
    m_watchdogThread.start();
    m_watchdogTimer = new QTimer;
    m_watchdogTimer->setInterval(std::chrono::seconds(2)); // max 2 secs allowed
    m_watchdogTimer->setSingleShot(true);
    m_watchdogTimer->moveToThread(&m_watchdogThread);
    QObject::connect(m_watchdogTimer, &QTimer::timeout, &m_interpreter, [this]() { m_interpreter.setInterrupted(true); }, Qt::DirectConnection);
    m_interpreter.installExtensions(QJSEngine::ConsoleExtension);

    m_jsapp = std::make_unique<JSApp>(m_doc, m_watchdogTimer);
    installGlobalCppProperty(QStringLiteral("app"), m_jsapp.get());
    m_jsdocument = std::make_unique<JSDocument>(m_doc, m_fieldCache);
    installGlobalCppProperty(QStringLiteral("Doc"), m_jsdocument.get());
    m_jsdisplay = std::make_unique<JSDisplay>();
    installGlobalCppProperty(QStringLiteral("display"), m_jsdisplay.get());
    m_jsspell = std::make_unique<JSSpell>();
    installGlobalCppProperty(QStringLiteral("spell"), m_jsspell.get());
    m_jsutil = std::make_unique<JSUtil>();
    installGlobalCppProperty(QStringLiteral("util"), m_jsutil.get());
    m_jsglobal = std::make_unique<JSGlobal>();
    installGlobalCppProperty(QStringLiteral("global"), m_jsglobal.get());
}

void ExecutorJSPrivate::updateEvent()
{
    if (!m_events.empty()) {
        const auto &top = m_events.top();
        if (top.first) {
            installGlobalCppProperty(QStringLiteral("event"), top.second.get());
        } else {
            m_interpreter.globalObject().setProperty(QStringLiteral("event"), QJSValue(QJSValue::UndefinedValue));
        }
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
    JSApp::clearCachedFields();
    delete d;
}

void ExecutorJS::execute(const QString &script, const std::shared_ptr<Event> &event)
{
    d->m_events.push(wrapEvent(event, d->m_fieldCache));
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
