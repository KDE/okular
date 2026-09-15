/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SCRIPT_EXECUTOR_JS_P_H
#define OKULAR_SCRIPT_EXECUTOR_JS_P_H

class QString;
#include <memory>

namespace Okular
{
class DocumentPrivate;
class ExecutorJSPrivate;
class Event;

class ExecutorJS
{
public:
    explicit ExecutorJS(DocumentPrivate *doc);
    ~ExecutorJS();

    ExecutorJS(const ExecutorJS &) = delete;
    ExecutorJS &operator=(const ExecutorJS &) = delete;

    void execute(const QString &script, const std::shared_ptr<Event> &event);

private:
    friend class ExecutorJSPrivate;
    ExecutorJSPrivate *d;
};

}

#endif
