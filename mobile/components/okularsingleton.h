/*
    SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULARSINGLETON_H
#define OKULARSINGLETON_H

#include <QObject>
#include <QStringList>

class OkularSingleton : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList nameFilters READ nameFilters CONSTANT)

public:
    OkularSingleton();

    QStringList nameFilters() const;
};

#endif
