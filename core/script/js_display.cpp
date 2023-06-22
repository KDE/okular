/*
    SPDX-FileCopyrightText: 2019 João Netto <joaonetto901@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../form.h"
#include "js_display_p.h"

#include <QString>

using namespace Okular;

// display.hidden
int JSDisplay::hidden() const
{
    return FormDisplay::FormHidden;
}

// display.visible
int JSDisplay::visible() const
{
    return FormDisplay::FormVisible;
}

// display.noView
int JSDisplay::noView() const
{
    return FormDisplay::FormNoView;
}

// display.noPrint
int JSDisplay::noPrint() const
{
    return FormDisplay::FormNoPrint;
}
