/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2008 by Harri Porten <porten@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "kjs_app_p.h"

#include <kjs/kjsarguments.h>
#include <kjs/kjsinterpreter.h>
#include <kjs/kjsobject.h>
#include <kjs/kjsprototype.h>
#include <kjs_version.h>

#include <QApplication>

#include <QLocale>
#include <QTimer>

#include <KLocalizedString>
#include <QCheckBox>
#include <QMessageBox>

#include "../document_p.h"
#include "../scripter.h"
#include "config-okular.h"
#include "kjs_fullscreen_p.h"

using namespace Okular;

#define OKULAR_TIMERID QStringLiteral("okular_timerID")

static KJSPrototype *g_appProto;
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

static KJSObject appGetFormsVersion(KJSContext *, void *)
{
    // faking a bit...
    return KJSNumber(fake_acroversion);
}

static KJSObject appGetLanguage(KJSContext *, void *)
{
    QLocale locale;
    QString lang = QLocale::languageToString(locale.language());
    QString country = QLocale::countryToString(locale.country());
    QString acroLang = QStringLiteral("ENU");
    if (lang == QLatin1String("da"))
        acroLang = QStringLiteral("DAN"); // Danish
    else if (lang == QLatin1String("de"))
        acroLang = QStringLiteral("DEU"); // German
    else if (lang == QLatin1String("en"))
        acroLang = QStringLiteral("ENU"); // English
    else if (lang == QLatin1String("es"))
        acroLang = QStringLiteral("ESP"); // Spanish
    else if (lang == QLatin1String("fr"))
        acroLang = QStringLiteral("FRA"); // French
    else if (lang == QLatin1String("it"))
        acroLang = QStringLiteral("ITA"); // Italian
    else if (lang == QLatin1String("ko"))
        acroLang = QStringLiteral("KOR"); // Korean
    else if (lang == QLatin1String("ja"))
        acroLang = QStringLiteral("JPN"); // Japanese
    else if (lang == QLatin1String("nl"))
        acroLang = QStringLiteral("NLD"); // Dutch
    else if (lang == QLatin1String("pt") && country == QLatin1String("BR"))
        acroLang = QStringLiteral("PTB"); // Brazilian Portuguese
    else if (lang == QLatin1String("fi"))
        acroLang = QStringLiteral("SUO"); // Finnish
    else if (lang == QLatin1String("sv"))
        acroLang = QStringLiteral("SVE"); // Swedish
    else if (lang == QLatin1String("zh") && country == QLatin1String("CN"))
        acroLang = QStringLiteral("CHS"); // Chinese Simplified
    else if (lang == QLatin1String("zh") && country == QLatin1String("TW"))
        acroLang = QStringLiteral("CHT"); // Chinese Traditional
    return KJSString(acroLang);
}

static KJSObject appGetNumPlugins(KJSContext *, void *)
{
    return KJSNumber(s_num_fake_plugins);
}

static KJSObject appGetPlatform(KJSContext *, void *)
{
#if defined(Q_OS_WIN)
    return KJSString(QString::fromLatin1("WIN"));
#elif defined(Q_OS_MAC)
    return KJSString(QString::fromLatin1("MAC"));
#else
    return KJSString(QStringLiteral("UNIX"));
#endif
}

static KJSObject appGetPlugIns(KJSContext *context, void *)
{
    KJSArray plugins(context, s_num_fake_plugins);
    for (int i = 0; i < s_num_fake_plugins; ++i) {
        const FakePluginInfo &info = s_fake_plugins[i];
        KJSObject plugin;
        plugin.setProperty(context, QStringLiteral("certified"), info.certified);
        plugin.setProperty(context, QStringLiteral("loaded"), info.loaded);
        plugin.setProperty(context, QStringLiteral("name"), info.name);
        plugin.setProperty(context, QStringLiteral("path"), info.path);
        plugin.setProperty(context, QStringLiteral("version"), fake_acroversion);
        plugins.setProperty(context, QString::number(i), plugin);
    }
    return plugins;
}

static KJSObject appGetPrintColorProfiles(KJSContext *context, void *)
{
    return KJSArray(context, 0);
}

static KJSObject appGetPrinterNames(KJSContext *context, void *)
{
    return KJSArray(context, 0);
}

static KJSObject appGetViewerType(KJSContext *, void *)
{
    // faking a bit...
    return KJSString(QStringLiteral("Reader"));
}

static KJSObject appGetViewerVariation(KJSContext *, void *)
{
    // faking a bit...
    return KJSString(QStringLiteral("Reader"));
}

static KJSObject appGetViewerVersion(KJSContext *, void *)
{
    // faking a bit...
    return KJSNumber(fake_acroversion);
}

/*
    Alert function defined in the reference, it shows a Dialog Box with options.
    app.alert()
*/
static KJSObject appAlert(KJSContext *context, void *, const KJSArguments &arguments)
{
    if (arguments.count() < 1) {
        return context->throwException(i18n("Missing alert type"));
    }
    QString cMsg = arguments.at(0).toString(context);
    int nIcon = 0;
    int nType = 0;
    QString cTitle = QStringLiteral("Okular");

    if (arguments.count() >= 2)
        nIcon = arguments.at(1).toInt32(context);
    if (arguments.count() >= 3)
        nType = arguments.at(2).toInt32(context);
    if (arguments.count() >= 4)
        cTitle = arguments.at(3).toString(context);

    QMessageBox::Icon icon;
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

    QMessageBox box(icon, cTitle, cMsg);

    switch (nType) {
    case 0:
        box.setStandardButtons(QMessageBox::Ok);
        break;
    case 1:
        box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        break;
    case 2:
        box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        break;
    case 3:
        box.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        break;
    }

    QCheckBox *checkBox = nullptr;
    KJSObject oCheckbox;
    if (arguments.count() >= 6) {
        oCheckbox = arguments.at(5);
        KJSObject oMsg = oCheckbox.property(context, QStringLiteral("cMsg"));
        QString msg = i18n("Do not show this message again");

        if (oMsg.isString())
            msg = oMsg.toString(context);

        bool bInitialValue = false;
        KJSObject value = oCheckbox.property(context, QStringLiteral("bInitialValue"));
        if (value.isBoolean())
            bInitialValue = value.toBoolean(context);
        checkBox = new QCheckBox(msg);
        checkBox->setChecked(bInitialValue);
        box.setCheckBox(checkBox);
    }

#if KJS_VERSION > QT_VERSION_CHECK(5, 71, 0)
    // halt timeout until the user has responded
    context->interpreter().stopTimeoutCheck();
#endif

    int button = box.exec();

#if KJS_VERSION > QT_VERSION_CHECK(5, 71, 0)
    // restart max allowed time
    context->interpreter().startTimeoutCheck();
#endif

    int ret;

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

    if (arguments.count() >= 6)
        oCheckbox.setProperty(context, QStringLiteral("bAfterValue"), checkBox->isChecked());

    delete checkBox;

    return KJSNumber(ret);
}

static KJSObject appBeep(KJSContext *context, void *, const KJSArguments &arguments)
{
    if (arguments.count() < 1) {
        return context->throwException(QStringLiteral("Missing beep type"));
    }
    QApplication::beep();
    return KJSUndefined();
}

static KJSObject appGetNthPlugInName(KJSContext *context, void *, const KJSArguments &arguments)
{
    if (arguments.count() < 1) {
        return context->throwException(QStringLiteral("Missing plugin index"));
    }
    const int nIndex = arguments.at(0).toInt32(context);

    if (nIndex < 0 || nIndex >= s_num_fake_plugins)
        return context->throwException(QStringLiteral("PlugIn index out of bounds"));

    const FakePluginInfo &info = s_fake_plugins[nIndex];
    return KJSString(info.name);
}

static KJSObject appGoBack(KJSContext *, void *object, const KJSArguments &)
{
    const DocumentPrivate *doc = reinterpret_cast<DocumentPrivate *>(object);
    if (doc->m_parent->historyAtBegin())
        return KJSUndefined();

    doc->m_parent->setPrevViewport();
    return KJSUndefined();
}

static KJSObject appGoForward(KJSContext *, void *object, const KJSArguments &)
{
    const DocumentPrivate *doc = reinterpret_cast<DocumentPrivate *>(object);
    if (doc->m_parent->historyAtEnd())
        return KJSUndefined();

    doc->m_parent->setNextViewport();
    return KJSUndefined();
}

// app.setInterval()
static KJSObject appSetInterval(KJSContext *ctx, void *object, const KJSArguments &arguments)
{
    DocumentPrivate *doc = reinterpret_cast<DocumentPrivate *>(object);
    const QString function = arguments.at(0).toString(ctx) + ';';
    const int interval = arguments.at(1).toInt32(ctx);

    QTimer *timer = new QTimer();

    QObject::connect(timer, &QTimer::timeout, doc->m_parent, [=]() { doc->executeScript(function); });

    timer->start(interval);

    return JSApp::wrapTimer(ctx, timer);
}

// app.clearInterval()
static KJSObject appClearInterval(KJSContext *ctx, void *, const KJSArguments &arguments)
{
    KJSObject timerObject = arguments.at(0);
    const int timerId = timerObject.property(ctx, OKULAR_TIMERID).toInt32(ctx);
    QTimer *timer = g_timerCache->value(timerId);
    if (timer != nullptr) {
        timer->stop();
        g_timerCache->remove(timerId);
        delete timer;
    }

    return KJSUndefined();
}

// app.setTimeOut()
static KJSObject appSetTimeOut(KJSContext *ctx, void *object, const KJSArguments &arguments)
{
    DocumentPrivate *doc = reinterpret_cast<DocumentPrivate *>(object);
    const QString function = arguments.at(0).toString(ctx) + ';';
    const int interval = arguments.at(1).toInt32(ctx);

    QTimer *timer = new QTimer();
    timer->setSingleShot(true);

    QObject::connect(timer, &QTimer::timeout, doc->m_parent, [=]() { doc->executeScript(function); });

    timer->start(interval);

    return JSApp::wrapTimer(ctx, timer);
}

// app.clearTimeOut()
static KJSObject appClearTimeOut(KJSContext *ctx, void *, const KJSArguments &arguments)
{
    KJSObject timerObject = arguments.at(0);
    const int timerId = timerObject.property(ctx, OKULAR_TIMERID).toInt32(ctx);
    QTimer *timer = g_timerCache->value(timerId);

    if (timer != nullptr) {
        timer->stop();
        g_timerCache->remove(timerId);
        delete timer;
    }

    return KJSUndefined();
}

void JSApp::initType(KJSContext *ctx)
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;

    g_appProto = new KJSPrototype();

    g_appProto->defineProperty(ctx, QStringLiteral("formsVersion"), appGetFormsVersion);
    g_appProto->defineProperty(ctx, QStringLiteral("language"), appGetLanguage);
    g_appProto->defineProperty(ctx, QStringLiteral("numPlugIns"), appGetNumPlugins);
    g_appProto->defineProperty(ctx, QStringLiteral("platform"), appGetPlatform);
    g_appProto->defineProperty(ctx, QStringLiteral("plugIns"), appGetPlugIns);
    g_appProto->defineProperty(ctx, QStringLiteral("printColorProfiles"), appGetPrintColorProfiles);
    g_appProto->defineProperty(ctx, QStringLiteral("printerNames"), appGetPrinterNames);
    g_appProto->defineProperty(ctx, QStringLiteral("viewerType"), appGetViewerType);
    g_appProto->defineProperty(ctx, QStringLiteral("viewerVariation"), appGetViewerVariation);
    g_appProto->defineProperty(ctx, QStringLiteral("viewerVersion"), appGetViewerVersion);

    g_appProto->defineFunction(ctx, QStringLiteral("alert"), appAlert);
    g_appProto->defineFunction(ctx, QStringLiteral("beep"), appBeep);
    g_appProto->defineFunction(ctx, QStringLiteral("getNthPlugInName"), appGetNthPlugInName);
    g_appProto->defineFunction(ctx, QStringLiteral("goBack"), appGoBack);
    g_appProto->defineFunction(ctx, QStringLiteral("goForward"), appGoForward);
    g_appProto->defineFunction(ctx, QStringLiteral("setInterval"), appSetInterval);
    g_appProto->defineFunction(ctx, QStringLiteral("clearInterval"), appClearInterval);
    g_appProto->defineFunction(ctx, QStringLiteral("setTimeOut"), appSetTimeOut);
    g_appProto->defineFunction(ctx, QStringLiteral("clearTimeOut"), appClearTimeOut);
}

KJSObject JSApp::object(KJSContext *ctx, DocumentPrivate *doc)
{
    return g_appProto->constructObject(ctx, doc);
}

KJSObject JSApp::wrapTimer(KJSContext *ctx, QTimer *timer)
{
    KJSObject timerObject = g_appProto->constructObject(ctx, timer);
    timerObject.setProperty(ctx, OKULAR_TIMERID, timer->timerId());

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
