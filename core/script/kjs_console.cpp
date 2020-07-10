/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2008 by Harri Porten <porten@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "kjs_console_p.h"

#include <kjs/kjsarguments.h>
#include <kjs/kjsobject.h>
#include <kjs/kjsprototype.h>

#include <QDebug>

#include "../debug_p.h"

using namespace Okular;

static KJSPrototype *g_consoleProto;

#ifdef OKULAR_JS_CONSOLE

#include <QLayout>
#include <QPlainTextEdit>

#include <KDialog>
#include <KStandardGuiItem>

K_GLOBAL_STATIC(KDialog, g_jsConsoleWindow)
static QPlainTextEdit *g_jsConsoleLog = 0;

static void createConsoleWindow()
{
    if (g_jsConsoleWindow.exists())
        return;

    g_jsConsoleWindow->setButtons(KDialog::Close | KDialog::User1);
    g_jsConsoleWindow->setButtonGuiItem(KDialog::User1, KStandardGuiItem::clear());

    QVBoxLayout *mainLay = new QVBoxLayout(g_jsConsoleWindow->mainWidget());
    mainLay->setContentsMargins(0, 0, 0, 0);
    g_jsConsoleLog = new QPlainTextEdit(g_jsConsoleWindow->mainWidget());
    g_jsConsoleLog->setReadOnly(true);
    mainLay->addWidget(g_jsConsoleLog);

    QObject::connect(g_jsConsoleWindow, SIGNAL(closeClicked()), g_jsConsoleWindow, SLOT(close()));
    QObject::connect(g_jsConsoleWindow, SIGNAL(user1Clicked()), g_jsConsoleLog, SLOT(clear()));
}

static void showConsole()
{
    createConsoleWindow();
    g_jsConsoleWindow->show();
}

static void hideConsole()
{
    if (!g_jsConsoleWindow.exists())
        return;

    g_jsConsoleWindow->hide();
}

static void clearConsole()
{
    if (!g_jsConsoleWindow.exists())
        return;

    g_jsConsoleLog->clear();
}

static void outputToConsole(const QString &message)
{
    showConsole();
    g_jsConsoleLog->appendPlainText(message);
}

#else /* OKULAR_JS_CONSOLE */

static void showConsole()
{
}

static void hideConsole()
{
}

static void clearConsole()
{
}

static void outputToConsole(const QString &cMessage)
{
    qCDebug(OkularCoreDebug) << "CONSOLE:" << cMessage;
}

#endif /* OKULAR_JS_CONSOLE */

static KJSObject consoleClear(KJSContext *, void *, const KJSArguments &)
{
    clearConsole();
    return KJSUndefined();
}

static KJSObject consoleHide(KJSContext *, void *, const KJSArguments &)
{
    hideConsole();
    return KJSUndefined();
}

static KJSObject consolePrintln(KJSContext *ctx, void *, const KJSArguments &arguments)
{
    QString cMessage = arguments.at(0).toString(ctx);
    outputToConsole(cMessage);

    return KJSUndefined();
}

static KJSObject consoleShow(KJSContext *, void *, const KJSArguments &)
{
    showConsole();
    return KJSUndefined();
}

void JSConsole::initType(KJSContext *ctx)
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;

    g_consoleProto = new KJSPrototype();

    g_consoleProto->defineFunction(ctx, QStringLiteral("clear"), consoleClear);
    g_consoleProto->defineFunction(ctx, QStringLiteral("hide"), consoleHide);
    g_consoleProto->defineFunction(ctx, QStringLiteral("println"), consolePrintln);
    g_consoleProto->defineFunction(ctx, QStringLiteral("hide"), consoleShow);
}

KJSObject JSConsole::object(KJSContext *ctx)
{
    return g_consoleProto->constructObject(ctx, nullptr);
}
