/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SCRIPT_JS_FIELD_P_H
#define OKULAR_SCRIPT_JS_FIELD_P_H

#include <QJSValue>
#include <QObject>

namespace Okular
{
class FormField;
class Page;

class JSField : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QJSValue doc READ doc CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(bool readonly READ readonly WRITE setReadonly) // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(QString type READ type CONSTANT)
    Q_PROPERTY(QJSValue value READ value WRITE setValue)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(QJSValue valueAsString READ valueAsString) // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(bool hidden READ hidden WRITE setHidden)   // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(int display READ display WRITE setDisplay) // clazy:exclude=qproperty-without-notify

public:
    explicit JSField(FormField *field, QObject *parent = nullptr);
    ~JSField() override;

    static QJSValue wrapField(QJSEngine *engine, FormField *field, Page *page);
    static void clearCachedFields();

    QJSValue doc() const;
    QString name() const;
    bool readonly() const;
    void setReadonly(bool readonly);
    int display() const;
    void setDisplay(int display);
    QString type() const;
    QJSValue value() const;
    void setValue(const QJSValue &value);
    QJSValue valueAsString() const;
    bool hidden() const;
    void setHidden(bool hidden);

    Q_INVOKABLE QJSValue buttonGetIcon(int nFace = 0) const;
    Q_INVOKABLE void buttonSetIcon(const QJSValue &oIcon, int nFace = 0);

private:
    QJSValue fieldGetValueCore(bool asString) const;

    FormField *m_field = nullptr;
};

}

#endif
