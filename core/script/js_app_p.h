/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SCRIPT_JS_APP_P_H
#define OKULAR_SCRIPT_JS_APP_P_H

#include <QJSValue>
#include <QObject>

class QTimer;

namespace Okular
{
class DocumentPrivate;

class JSApp : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int formsVersion READ formsVersion CONSTANT)
    Q_PROPERTY(QString language READ language CONSTANT)
    Q_PROPERTY(int numPlugIns READ numPlugIns CONSTANT)
    Q_PROPERTY(QString platform READ platform CONSTANT)
    Q_PROPERTY(QJSValue plugIns READ plugIns CONSTANT)
    Q_PROPERTY(QStringList printColorProfiles READ printColorProfiles CONSTANT)
    Q_PROPERTY(QStringList printerNames READ printerNames CONSTANT)
    Q_PROPERTY(QString viewerType READ viewerType CONSTANT)
    Q_PROPERTY(QString viewerVariation READ viewerVariation CONSTANT)
    Q_PROPERTY(int viewerVersion READ viewerVersion CONSTANT)

public:
    explicit JSApp(DocumentPrivate *doc, QTimer *watchdogTimer, QObject *parent = nullptr);
    ~JSApp() override;
    static void clearCachedFields();

    int formsVersion() const;
    QString language() const;
    int numPlugIns() const;
    QString platform() const;
    QJSValue plugIns() const;
    QStringList printColorProfiles() const;
    QStringList printerNames() const;
    QString viewerType() const;
    QString viewerVariation() const;
    int viewerVersion() const;

    Q_INVOKABLE int alert(const QJSValue &arguments);
    Q_INVOKABLE int alert(const QString &cMsg, int nIcon = 0, int nType = 0, const QString &cTitle = {}, const QJSValue &oDoc = {}, const QJSValue &oCheckbox = {});
    Q_INVOKABLE void beep(int nType = 4);
    Q_INVOKABLE QJSValue getNthPlugInName(int nIndex) const;
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void goForward();
    Q_INVOKABLE QJSValue setInterval(const QString &cExpr, int nMilliseconds);
    Q_INVOKABLE void clearInterval(const QJSValue &oInterval);
    Q_INVOKABLE QJSValue setTimeOut(const QString &cExpr, int nMilliseconds);
    Q_INVOKABLE void clearTimeOut(const QJSValue &oTime);

private:
    QJSValue wrapTimer(QTimer *timer) const;

    DocumentPrivate *m_doc = nullptr;
    QTimer *m_watchdogTimer = nullptr;
};

}

#endif
