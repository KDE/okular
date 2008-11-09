/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_TEXTEDITORS_P_H
#define OKULAR_TEXTEDITORS_P_H

#include "settings.h"

#include <qhash.h>
#include <qstring.h>

namespace Okular
{

static inline QHash< int, QString > buildEditorsMap()
{
    QHash< int, QString > editors;
    editors.insert( Settings::EnumExternalEditor::Kate,
        QString::fromLatin1( "kate --use --line %l --column %c" ) );
    editors.insert( Settings::EnumExternalEditor::Kile,
        QString::fromLatin1( "kile --line %l" ) );
    editors.insert( Settings::EnumExternalEditor::Scite,
        QString::fromLatin1( "scite %f \"-goto:%l,%c\"" ) );
    editors.insert( Settings::EnumExternalEditor::Emacsclient,
        QString::fromLatin1( "emacsclient -a emacs --no-wait +%l %f" ) );
    return editors;
}

}

#endif
