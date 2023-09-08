/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "js_app_p.h"

#include <QApplication>

#include <QLocale>
#include <QTimer>

#include <KLocalizedString>
#include <QCheckBox>
#include <QJSEngine>
#include <QMessageBox>

#include "../document_p.h"
#include "../scripter.h"
#include "config-okular.h"
#include "js_fullscreen_p.h"

using namespace Okular;

#define OKULAR_TIMERID QStringLiteral("okular_timerID")

typedef QHash<int, QTimer *> TimerCache;
Q_GLOBAL_STATIC(TimerCache, g_timerCache)

// the acrobat version we fake
static const double fake_acroversion = 8.00;

static const struct FakePluginInfo {
    const char *name;
    bool certified;
    bool loaded;
    const char *path;
} s_fake_plugins[] = {{"Annots", true, true, ""}, {"EFS", true, true, ""}, {"EScript", true, true, ""}, {"Forms", true, true, ""}, {"ReadOutLoud", true, true, ""}, {"WebLink", true, true, ""}};
static const int s_num_fake_plugins = sizeof(s_fake_plugins) / sizeof(s_fake_plugins[0]);

int JSApp::formsVersion() const
{
    // faking a bit...
    return fake_acroversion;
}

QString JSApp::language() const
{
    QLocale locale;
    QString lang = QLocale::languageToString(locale.language());
    QString country = QLocale::countryToString(locale.country());
    QString acroLang = QStringLiteral("ENU");
    if (lang == QLatin1String("da")) {
        acroLang = QStringLiteral("DAN"); // Danish
    } else if (lang == QLatin1String("de")) {
        acroLang = QStringLiteral("DEU"); // German
    } else if (lang == QLatin1String("en")) {
        acroLang = QStringLiteral("ENU"); // English
    } else if (lang == QLatin1String("es")) {
        acroLang = QStringLiteral("ESP"); // Spanish
    } else if (lang == QLatin1String("fr")) {
        acroLang = QStringLiteral("FRA"); // French
    } else if (lang == QLatin1String("it")) {
        acroLang = QStringLiteral("ITA"); // Italian
    } else if (lang == QLatin1String("ko")) {
        acroLang = QStringLiteral("KOR"); // Korean
    } else if (lang == QLatin1String("ja")) {
        acroLang = QStringLiteral("JPN"); // Japanese
    } else if (lang == QLatin1String("nl")) {
        acroLang = QStringLiteral("NLD"); // Dutch
    } else if (lang == QLatin1String("pt") && country == QLatin1String("BR")) {
        acroLang = QStringLiteral("PTB"); // Brazilian Portuguese
    } else if (lang == QLatin1String("fi")) {
        acroLang = QStringLiteral("SUO"); // Finnish
    } else if (lang == QLatin1String("sv")) {
        acroLang = QStringLiteral("SVE"); // Swedish
    } else if (lang == QLatin1String("zh") && country == QLatin1String("CN")) {
        acroLang = QStringLiteral("CHS"); // Chinese Simplified
    } else if (lang == QLatin1String("zh") && country == QLatin1String("TW")) {
        acroLang = QStringLiteral("CHT"); // Chinese Traditional
    }
    return acroLang;
}

int JSApp::numPlugIns() const
{
    return s_num_fake_plugins;
}

QString JSApp::platform() const
{
#if defined(Q_OS_WIN)
    return QString::fromLatin1("WIN");
#elif defined(Q_OS_MAC)
    return QString::fromLatin1("MAC");
#else
    return QStringLiteral("UNIX");
#endif
}

QJSValue JSApp::plugIns() const
{
    QJSValue plugins = qjsEngine(this)->newArray(s_num_fake_plugins);
    for (int i = 0; i < s_num_fake_plugins; ++i) {
        const FakePluginInfo &info = s_fake_plugins[i];
        QJSValue plugin = qjsEngine(this)->newObject();
        plugin.setProperty(QStringLiteral("certified"), info.certified);
        plugin.setProperty(QStringLiteral("loaded"), info.loaded);
        plugin.setProperty(QStringLiteral("name"), info.name);
        plugin.setProperty(QStringLiteral("path"), info.path);
        plugin.setProperty(QStringLiteral("version"), fake_acroversion);
        plugins.setProperty(i, plugin);
    }
    return plugins;
}

QStringList JSApp::printColorProfiles() const
{
    return QStringList();
}

QStringList JSApp::printerNames() const
{
    return QStringList();
}

QString JSApp::viewerType() const
{
    // faking a bit...
    return QStringLiteral("Reader");
}

QString JSApp::viewerVariation() const
{
    // faking a bit...
    return QStringLiteral("Reader");
}

int JSApp::viewerVersion() const
{
    // faking a bit...
    return fake_acroversion;
}

/*
    Alert function defined in the reference, it shows a Dialog Box with options.
    app.alert()
*/
int JSApp::alert(const QJSValue &arguments)
{
    const auto cMsg = arguments.property(QStringLiteral("cMsg")).toString();
    const auto nIcon = arguments.property(QStringLiteral("nIcon")).toInt();
    const auto nType = arguments.property(QStringLiteral("nType")).toInt();
    const auto cTitle = arguments.property(QStringLiteral("cTitle")).toString();
    const auto oCheckbox = arguments.property(QStringLiteral("oCheckbox"));
    return alert(cMsg, nIcon, nType, cTitle, QJSValue(), oCheckbox);
}

int JSApp::alert(const QString &cMsg, int nIcon, int nType, const QString &cTitle, [[maybe_unused]] const QJSValue &oDoc, const QJSValue &oCheckbox)
{
    QMessageBox::Icon icon = QMessageBox::Critical;
    switch (nIcon) {
    case 0:
        icon = QMessageBox::Critical;
        break;
    case 1:
        icon = QMessageBox::Warning;
        break;
    case 2:
        icon = QMessageBox::Question;
        break;
    case 3:
        icon = QMessageBox::Information;
        break;
    }

    const QString title = !cTitle.isEmpty() ? cTitle : QStringLiteral("Okular");
    QMessageBox box(icon, title, cMsg);

    QMessageBox::StandardButtons buttons = QMessageBox::Ok;
    switch (nType) {
    case 0:
        buttons = QMessageBox::Ok;
        break;
    case 1:
        buttons = QMessageBox::Ok | QMessageBox::Cancel;
        break;
    case 2:
        buttons = QMessageBox::Yes | QMessageBox::No;
        break;
    case 3:
        buttons = QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel;
        break;
    }
    box.setStandardButtons(buttons);

    QCheckBox *checkBox = nullptr;
    if (oCheckbox.isObject()) {
        const auto oMsg = oCheckbox.property(QStringLiteral("cMsg"));
        QString msg = i18n("Do not show this message again");

        if (oMsg.isString()) {
            msg = oMsg.toString();
        }

        bool bInitialValue = false;
        const auto value = oCheckbox.property(QStringLiteral("bInitialValue"));
        if (value.isBool()) {
            bInitialValue = value.toBool();
        }
        checkBox = new QCheckBox(msg);
        checkBox->setChecked(bInitialValue);
        box.setCheckBox(checkBox);
    }

    // halt timeout until the user has responded
    QMetaObject::invokeMethod(m_watchdogTimer, qOverload<>(&QTimer::stop));

    int button = box.exec();

    // restart max allowed time
    QMetaObject::invokeMethod(m_watchdogTimer, qOverload<>(&QTimer::start));

    int ret = 0;

    switch (button) {
    case QMessageBox::Ok:
        ret = 1;
        break;
    case QMessageBox::Cancel:
        ret = 2;
        break;
    case QMessageBox::No:
        ret = 3;
        break;
    case QMessageBox::Yes:
        ret = 4;
        break;
    }

    if (checkBox) {
        QJSValue(oCheckbox).setProperty(QStringLiteral("bAfterValue"), checkBox->isChecked());
    }

    delete checkBox;

    return ret;
}

void JSApp::beep([[maybe_unused]] int nType)
{
    QApplication::beep();
}

QJSValue JSApp::getNthPlugInName(int nIndex) const
{
    if (nIndex < 0 || nIndex >= s_num_fake_plugins) {
        return qjsEngine(this)->newErrorObject(QJSValue::TypeError, QStringLiteral("PlugIn index out of bounds"));
    }

    const FakePluginInfo &info = s_fake_plugins[nIndex];
    return info.name;
}

void JSApp::goBack()
{
    if (!m_doc->m_parent->historyAtBegin()) {
        m_doc->m_parent->setPrevViewport();
    }
}

void JSApp::goForward()
{
    if (!m_doc->m_parent->historyAtEnd()) {
        m_doc->m_parent->setNextViewport();
    }
}

// app.setInterval()
QJSValue JSApp::setInterval(const QString &cExpr, int nMilliseconds)
{
    QTimer *timer = new QTimer();

    QObject::connect(timer, &QTimer::timeout, m_doc->m_parent, [=]() { m_doc->executeScript(cExpr); });

    timer->start(nMilliseconds);

    return JSApp::wrapTimer(timer);
}

// app.clearInterval()
void JSApp::clearInterval(const QJSValue &oInterval)
{
    const int timerId = oInterval.property(OKULAR_TIMERID).toInt();
    QTimer *timer = g_timerCache->value(timerId);
    if (timer != nullptr) {
        timer->stop();
        g_timerCache->remove(timerId);
        delete timer;
    }
}

// app.setTimeOut()
QJSValue JSApp::setTimeOut(const QString &cExpr, int nMilliseconds)
{
    QTimer *timer = new QTimer();
    timer->setSingleShot(true);

    QObject::connect(timer, &QTimer::timeout, m_doc->m_parent, [=]() { m_doc->executeScript(cExpr); });

    timer->start(nMilliseconds);

    return JSApp::wrapTimer(timer);
}

// app.clearTimeOut()
void JSApp::clearTimeOut(const QJSValue &oTime)
{
    const int timerId = oTime.property(OKULAR_TIMERID).toInt();
    QTimer *timer = g_timerCache->value(timerId);

    if (timer != nullptr) {
        timer->stop();
        g_timerCache->remove(timerId);
        delete timer;
    }
}

JSApp::JSApp(DocumentPrivate *doc, QTimer *watchdogTimer, QObject *parent)
    : QObject(parent)
    , m_doc(doc)
    , m_watchdogTimer(watchdogTimer)
{
}

JSApp::~JSApp() = default;

QJSValue JSApp::wrapTimer(QTimer *timer) const
{
    QJSValue timerObject = qjsEngine(this)->newObject();
    timerObject.setProperty(OKULAR_TIMERID, timer->timerId());

    g_timerCache->insert(timer->timerId(), timer);

    return timerObject;
}

void JSApp::clearCachedFields()
{
    if (g_timerCache) {
        qDeleteAll(g_timerCache->begin(), g_timerCache->end());
        g_timerCache->clear();
    }
}
