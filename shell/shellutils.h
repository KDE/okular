/***************************************************************************
 *   Copyright (C) 2009 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_SHELLUTILS_H
#define OKULAR_SHELLUTILS_H

#include <QString>

class QUrl;

namespace ShellUtils
{

typedef bool (*FileExistFunc)( const QString& fileName );

FileExistFunc qfileExistFunc();
QUrl urlFromArg( const QString& _arg, FileExistFunc exist_func, const QString& pageArg = QString() );

}

#endif
