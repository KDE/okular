/***************************************************************************
 *   Copyright (C) 2008 by Ely Levy <elylevy@cs.huji.ac.il>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include "generator_epub.h"

#include "converter.h"

#include <kaboutdata.h>

static KAboutData createAboutData()
{
    // ### TODO fill after the KDE 4.0 unfreeze
    KAboutData aboutData(
         "okular_epub",
         "okular_epub",
         KLocalizedString(),
         "0.1",
         KLocalizedString(),
         KAboutData::License_GPL,
         KLocalizedString()
    );
    return aboutData;
}

OKULAR_EXPORT_PLUGIN( EPubGenerator, createAboutData() )

EPubGenerator::EPubGenerator( QObject *parent, const QVariantList &args )
: Okular::TextDocumentGenerator( new EPub::Converter, parent, args )
{
}
