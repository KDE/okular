/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_SCRIPTER_H
#define OKULAR_SCRIPTER_H

#include "global.h"

class QString;

namespace Okular
{
class Document;
class DocumentPrivate;
class Event;
class ScripterPrivate;

class Scripter
{
    friend class Document;
    friend class DocumentPrivate;

public:
    ~Scripter();

    Scripter(const Scripter &) = delete;
    Scripter &operator=(const Scripter &) = delete;

    void execute(ScriptType type, const QString &script);

    void setEvent(Event *event);
    Event *event() const;

private:
    friend class ScripterPrivate;
    ScripterPrivate *d;

    explicit Scripter(DocumentPrivate *doc);
};

}

#endif
