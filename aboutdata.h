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

inline KAboutData* okularAboutData( const char* name, const char* iname )
{
    KAboutData *about = new KAboutData(
        name, //"okular",
        iname, //I18N_NOOP("okular"),
        "0.5.82",
        I18N_NOOP("okular, an universal document viewer"),
        KAboutData::License_GPL,
        "(C) 2002 Wilco Greven, Christophe Devriese\n"
        "(C) 2004-2005 Albert Astals Cid, Enrico Ros\n"
        "(C) 2005 Piotr Szymanski"
    );

    about->addAuthor("Pino Toscano", I18N_NOOP("Current maintainer"), "pino@kde.org");
    about->addAuthor("Tobias Koenig", I18N_NOOP("Lots of framework work, ODT and FictionBook backends"), "tokoe@kde.org");
    about->addAuthor("Albert Astals Cid", I18N_NOOP("Former maintainer"), "aacid@kde.org");
    about->addAuthor("Piotr Szymanski", I18N_NOOP("Created okular from KPDF codebase"), "djurban@pld-dc.org");
    about->addAuthor("Enrico Ros", 0, "eros.kde@email.it");
    about->addAuthor("Wilco Greven", 0, "greven@kde.org");
    about->addAuthor("Christophe Devriese", 0, "oelewapperke@oelewapperke.org");
    about->addAuthor("Laurent Montel", 0, "montel@kde.org");

    about->addCredit("Marco Martin", I18N_NOOP("Icon"), "m4rt@libero.it");

    return about;
}

#endif
