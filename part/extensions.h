/*
    SPDX-FileCopyrightText: 2002 Wilco Greven <greven@kde.org>
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _EXTENSIONS_H_
#define _EXTENSIONS_H_

#include <QtGlobal>

#include <KParts/NavigationExtension>
using NavigationExtension = KParts::NavigationExtension;

namespace Okular
{
class Part;

class BrowserExtension : public NavigationExtension
{
    Q_OBJECT

public:
    explicit BrowserExtension(Part *);

public Q_SLOTS:
    // Automatically detected by the host.
    void print();

private:
    Part *m_part;
};

}

#endif

/* kate: replace-tabs on; indent-width 4; */
