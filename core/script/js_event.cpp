/*
    SPDX-FileCopyrightText: 2018 Intevation GmbH <intevation@intevation.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "js_event_p.h"

#include "event_p.h"
#include "js_field_p.h"

#include <QJSEngine>

using namespace Okular;

// Event.name
QString JSEvent::name() const
{
    return m_event->name();
}

// Event.type
QString JSEvent::type() const
{
    return m_event->type();
}

// Event.targetName (getter)
QString JSEvent::targetName() const
{
    return m_event->targetName();
}

// Event.targetName (setter)
void JSEvent::setTargetName(const QString &targetName)
{
    m_event->setTargetName(targetName);
}

// Event.shift
bool JSEvent::shift() const
{
    return m_event->shiftModifier();
}

// Event.source
QJSValue JSEvent::source() const
{
    if (m_event->eventType() == Event::FieldCalculate) {
        FormField *src = m_event->source();
        if (src) {
            return JSField::wrapField(qjsEngine(this), src, m_event->sourcePage());
        }
    }
    return QJSValue(QJSValue::UndefinedValue);
}

// Event.target
QJSValue JSEvent::target() const
{
    switch (m_event->eventType()) {
    case Event::FieldCalculate:
    case Event::FieldFormat:
    case Event::FieldKeystroke:
    case Event::FieldFocus:
    case Event::FieldValidate: {
        FormField *target = static_cast<FormField *>(m_event->target());
        if (target) {
            return JSField::wrapField(qjsEngine(this), target, m_event->targetPage());
        }
        break;
    }
    default: {
    }
    }
    return QJSValue(QJSValue::UndefinedValue);
}

// Event.value (getter)
QJSValue JSEvent::value() const
{
    return m_event->value().toString();
}

// Event.value (setter)
void JSEvent::setValue(const QJSValue &value)
{
    m_event->setValue(QVariant(value.toString()));
}

// Event.rc (getter)
bool JSEvent::returnCode() const
{
    return m_event->returnCode();
}

// Event.rc (setter)
void JSEvent::setReturnCode(bool rc)
{
    m_event->setReturnCode(rc);
}

// Event.willCommit (getter)
bool JSEvent::willCommit() const
{
    return m_event->willCommit();
}

// Event.change (getter)
QString JSEvent::change() const
{
    return m_event->change();
}

JSEvent::JSEvent(Event *event, QObject *parent)
    : QObject(parent)
    , m_event(event)
{
}

JSEvent::~JSEvent() = default;
