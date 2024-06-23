/*
    SPDX-FileCopyrightText: 2024 Pratham Gandhi <ppg.1382@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SCRIPT_JS_GLOBAL_P_H
#define OKULAR_SCRIPT_JS_GLOBAL_P_H

#include <QObject>

namespace Okular
{

class JSGlobal : public QObject
{
    Q_OBJECT

public:
    explicit JSGlobal(QObject *parent = nullptr);
    ~JSGlobal() override;
};

}
#endif