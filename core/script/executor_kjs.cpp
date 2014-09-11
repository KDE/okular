/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2008 by Harri Porten <porten@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "executor_kjs_p.h"

#include <kjs/kjsinterpreter.h>
#include <kjs/kjsobject.h>
#include <kjs/kjsprototype.h>
#include <kjs/kjsarguments.h>

#include <kdebug.h>

#include "../debug_p.h"
#include "../document_p.h"

#include "kjs_app_p.h"
#include "kjs_console_p.h"
#include "kjs_data_p.h"
#include "kjs_document_p.h"
#include "kjs_field_p.h"
#include "kjs_fullscreen_p.h"
#include "kjs_spell_p.h"
#include "kjs_util_p.h"

using namespace Okular;

class Okular::ExecutorKJSPrivate
{
    public:
        ExecutorKJSPrivate( DocumentPrivate *doc )
            : m_doc( doc )
        {
            initTypes();
        }
        ~ExecutorKJSPrivate()
        {
            JSField::clearCachedFields();

            delete m_interpreter;
        }

        void initTypes();

        DocumentPrivate *m_doc;
        KJSInterpreter *m_interpreter;
        KJSGlobalObject m_docObject;
};

void ExecutorKJSPrivate::initTypes()
{
    m_docObject = JSDocument::wrapDocument( m_doc );
    m_interpreter = new KJSInterpreter( m_docObject );

    KJSContext *ctx = m_interpreter->globalContext();

    JSApp::initType( ctx );
    JSFullscreen::initType( ctx );
    JSConsole::initType( ctx );
    JSData::initType( ctx );
    JSDocument::initType( ctx );
    JSField::initType( ctx );
    JSSpell::initType( ctx );
    JSUtil::initType( ctx );

    m_docObject.setProperty( ctx, "app", JSApp::object( ctx, m_doc ) );
    m_docObject.setProperty( ctx, "console", JSConsole::object( ctx ) );
    m_docObject.setProperty( ctx, "Doc", m_docObject );
    m_docObject.setProperty( ctx, "spell", JSSpell::object( ctx ) );
    m_docObject.setProperty( ctx, "util", JSUtil::object( ctx ) );
}

ExecutorKJS::ExecutorKJS( DocumentPrivate *doc )
    : d( new ExecutorKJSPrivate( doc ) )
{
}

ExecutorKJS::~ExecutorKJS()
{
    delete d;
}

void ExecutorKJS::execute( const QString &script )
{
#if 0
    QString script2;
    QString errMsg;
    int errLine;
    if ( !KJSInterpreter::normalizeCode( script, &script2, &errLine, &errMsg ) )
    {
        kWarning(OkularDebug) << "Parse error during normalization!";
        script2 = script;
    }
#endif

    KJSResult result = d->m_interpreter->evaluate( "okular.js", 1,
                                                   script, &d->m_docObject );
    KJSContext* ctx = d->m_interpreter->globalContext();
    if ( result.isException() || ctx->hasException() )
    {
        kDebug(OkularDebug) << "JS exception" << result.errorMessage();
    }
    else
    {
        kDebug(OkularDebug) << "result:" << result.value().toString( ctx );
    }
}
