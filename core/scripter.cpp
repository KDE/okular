/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "scripter.h"

#include <kdebug.h>

#include "debug_p.h"
#include "script/executor_kjs_p.h"

using namespace Okular;

class Okular::ScripterPrivate
{
    public:
        ScripterPrivate( DocumentPrivate *doc )
            : m_doc( doc ), m_kjs( 0 )
        {
        }

        ~ScripterPrivate()
        {
            delete m_kjs;
        }

        DocumentPrivate *m_doc;
        ExecutorKJS *m_kjs;
};

Scripter::Scripter( DocumentPrivate *doc )
    : d( new ScripterPrivate( doc ) )
{
}

Scripter::~Scripter()
{
    delete d;
}

QString Scripter::execute( ScriptType type, const QString &script )
{
    kDebug(OkularDebug) << "executing the script:";
#if 0
    if ( script.length() < 1000 )
        qDebug() << script;
    else
        qDebug() << script.left( 1000 ) << "[...]";
#endif
    switch ( type )
    {
        case JavaScript:
            if ( !d->m_kjs )
            {
                d->m_kjs = new ExecutorKJS( d->m_doc );
            }
            d->m_kjs->execute( script );
            break;
    }
    return QString();
}
