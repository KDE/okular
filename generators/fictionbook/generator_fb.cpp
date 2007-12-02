/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator_fb.h"

#include "converter.h"

#include <kaboutdata.h>

static KAboutData createAboutData()
{
    // ### TODO fill after the KDE 4.0 unfreeze
    KAboutData aboutData(
         "okular_fictionbook",
         "okular_fictionbook",
         KLocalizedString(),
         "0.1",
         KLocalizedString(),
         KAboutData::License_GPL,
         KLocalizedString()
    );
    return aboutData;
}

OKULAR_EXPORT_PLUGIN( FictionBookGenerator, createAboutData() )

FictionBookGenerator::FictionBookGenerator( QObject *parent, const QVariantList &args )
    : Okular::TextDocumentGenerator( new FictionBook::Converter, parent, args )
{
}
