/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_SCRIPT_EXECUTOR_KJS_P_H
#define OKULAR_SCRIPT_EXECUTOR_KJS_P_H

class QString;

namespace Okular {

class DocumentPrivate;
class ExecutorKJSPrivate;

class ExecutorKJS
{
    public:
        ExecutorKJS( DocumentPrivate *doc );
        ~ExecutorKJS();

        void execute( const QString &script );

    private:
        friend class ExecutorKJSPrivate;
        ExecutorKJSPrivate* d;
};

}

#endif
