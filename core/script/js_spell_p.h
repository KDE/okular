/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SCRIPT_JS_SPELL_P_H
#define OKULAR_SCRIPT_JS_SPELL_P_H

#include <QObject>

namespace Okular
{
class JSSpell : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
public:
    bool available() const;
};

}

#endif
