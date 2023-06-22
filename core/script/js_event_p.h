/*
    SPDX-FileCopyrightText: 2018 Intevation GmbH <intevation@intevation.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SCRIPT_JS_EVENT_P_H
#define OKULAR_SCRIPT_JS_EVENT_P_H

#include <QJSValue>
#include <QObject>

namespace Okular
{
class Event;

class JSEvent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString type READ type CONSTANT)
    Q_PROPERTY(QString targetName READ targetName WRITE setTargetName) // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(bool shift READ shift CONSTANT)
    Q_PROPERTY(QJSValue source READ source CONSTANT)
    Q_PROPERTY(QJSValue target READ target CONSTANT)
    Q_PROPERTY(bool willCommit READ willCommit CONSTANT)
    Q_PROPERTY(QJSValue value READ value WRITE setValue)    // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(bool rc READ returnCode WRITE setReturnCode) // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(QString change READ change CONSTANT)

public:
    explicit JSEvent(Event *event, QObject *parent = nullptr);
    ~JSEvent() override;

    QString name() const;
    QString type() const;
    QString targetName() const;
    void setTargetName(const QString &targetName);
    bool shift() const;
    QJSValue source() const;
    QJSValue target() const;
    bool willCommit() const;
    QJSValue value() const;
    void setValue(const QJSValue &value);
    bool returnCode() const;
    void setReturnCode(bool rc);
    QString change() const;

private:
    Event *m_event = nullptr;
};

}

#endif
