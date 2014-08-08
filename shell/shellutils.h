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

#include <qstring.h>

#include <kurl.h>

class KCmdLineArgs;

namespace ShellUtils
{

typedef bool (*FileExistFunc)( const QString& fileName );

FileExistFunc qfileExistFunc();
KUrl urlFromArg( const QString& _arg, FileExistFunc exist_func, const QString& pageArg = QString() );
QString serializeOptions(const KCmdLineArgs &args);
QString serializeOptions(bool startInPresentation, bool showPrintDialog, bool unique, bool noRaise, const QString &page);
bool unserializeOptions(const QString &serializedOptions, bool *presentation, bool *print, bool *unique, bool *noraise, QString *page);
bool unique(const QString &serializedOptions);
bool noRaise(const QString &serializedOptions);
bool startInPresentation(const QString &serializedOptions);
bool showPrintDialog(const QString &serializedOptions);
QString page(const QString &serializedOptions);

}

#endif
