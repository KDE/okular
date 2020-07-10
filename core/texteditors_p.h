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

#include "settings_core.h"

#include <QHash>
#include <QString>

namespace Okular
{
static inline QHash<int, QString> buildEditorsMap()
{
    QHash<int, QString> editors;
    editors.insert(SettingsCore::EnumExternalEditor::Kate, QStringLiteral("kate --line %l --column %c"));
    editors.insert(SettingsCore::EnumExternalEditor::Kile, QStringLiteral("kile --line %l"));
    editors.insert(SettingsCore::EnumExternalEditor::Scite, QStringLiteral("scite %f \"-goto:%l,%c\""));
    editors.insert(SettingsCore::EnumExternalEditor::Emacsclient, QStringLiteral("emacsclient -a emacs --no-wait +%l %f"));
    editors.insert(SettingsCore::EnumExternalEditor::Lyxclient, QStringLiteral("lyxclient -g %f %l"));
    editors.insert(SettingsCore::EnumExternalEditor::Texstudio, QStringLiteral("texstudio --line %l"));
    editors.insert(SettingsCore::EnumExternalEditor::Texifyidea, QStringLiteral("idea --line %l"));
    return editors;
}

}

#endif
