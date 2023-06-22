/*
    SPDX-FileCopyrightText: 2019 João Netto <joaonetto901@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SCRIPT_JS_DISPLAY_P_H
#define OKULAR_SCRIPT_JS_DISPLAY_P_H

#include <QObject>

namespace Okular
{
/**
 * The display types of the field.
 */
enum FormDisplay { FormVisible, FormHidden, FormNoPrint, FormNoView };

class JSDisplay : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int hidden READ hidden CONSTANT)
    Q_PROPERTY(int visible READ visible CONSTANT)
    Q_PROPERTY(int noView READ noView CONSTANT)
    Q_PROPERTY(int noPrint READ noPrint CONSTANT)
public:
    int hidden() const;
    int visible() const;
    int noView() const;
    int noPrint() const;
};

}

#endif
