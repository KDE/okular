/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "js_console_p.h"

#include <QDebug>

#include "../debug_p.h"

using namespace Okular;

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
#endif

void JSConsole::show()
{
#ifdef OKULAR_JS_CONSOLE
    createConsoleWindow();
    g_jsConsoleWindow->show();
#endif
}

void JSConsole::hide()
{
#ifdef OKULAR_JS_CONSOLE
    if (!g_jsConsoleWindow.exists())
        return;

    g_jsConsoleWindow->hide();
#endif
}

void JSConsole::clear()
{
#ifdef OKULAR_JS_CONSOLE
    if (!g_jsConsoleWindow.exists())
        return;

    g_jsConsoleLog->clear();
#endif
}

void JSConsole::println(const QString &cMessage)
{
#ifdef OKULAR_JS_CONSOLE
    showConsole();
    g_jsConsoleLog->appendPlainText(cMessage);
#else
    Q_UNUSED(cMessage);
#endif
}
