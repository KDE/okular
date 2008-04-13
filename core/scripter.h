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
class QStringList;

namespace Okular {

class Document;
class DocumentPrivate;
class ScripterPrivate;

class Scripter
{
    friend class Document;
    friend class DocumentPrivate;

    public:
        ~Scripter();

        QString execute( ScriptType type, const QString &script );

    private:
        friend class ScripterPrivate;
        ScripterPrivate* d;

        Scripter( DocumentPrivate *doc );
};

}

#endif
