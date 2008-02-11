/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _ABOUTDATA_H_
#define _ABOUTDATA_H_

#include <kaboutdata.h>

#include "core/version.h"

inline KAboutData okularAboutData( const char* name, const char* iname )
{
    KAboutData about(
        name, //"okular",
        "okular",
        ki18n(iname), //I18N_NOOP("okular"),
        OKULAR_VERSION_STRING,
        ki18n("okular, a universal document viewer"),
        KAboutData::License_GPL,
        ki18n("(C) 2002 Wilco Greven, Christophe Devriese\n"
              "(C) 2004-2005 Albert Astals Cid, Enrico Ros\n"
              "(C) 2005 Piotr Szymanski"),
        KLocalizedString(),
        "http://okular.kde.org"
    );

    about.addAuthor(ki18n("Pino Toscano"), ki18n("Current maintainer"), "pino@kde.org");
    about.addAuthor(ki18n("Tobias Koenig"), ki18n("Lots of framework work, ODT and FictionBook backends"), "tokoe@kde.org");
    about.addAuthor(ki18n("Albert Astals Cid"), ki18n("Former maintainer"), "aacid@kde.org");
    about.addAuthor(ki18n("Piotr Szymanski"), ki18n("Created okular from KPDF codebase"), "djurban@pld-dc.org");
    about.addAuthor(ki18n("Enrico Ros"), ki18n("KPDF developer"), "eros.kde@email.it");

    return about;
}

#endif
