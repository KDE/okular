/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator_ooo.h"

#include "converter.h"

#include <kaboutdata.h>

static KAboutData createAboutData()
{
    // ### TODO fill after the KDE 4.0 unfreeze
    KAboutData aboutData(
         "okular_ooo",
         "okular_ooo",
         KLocalizedString(),
         "0.1",
         KLocalizedString(),
         KAboutData::License_GPL,
         KLocalizedString()
    );
    return aboutData;
}

OKULAR_EXPORT_PLUGIN( KOOOGenerator, createAboutData() )

KOOOGenerator::KOOOGenerator( QObject *parent, const QVariantList &args )
  : Okular::TextDocumentGenerator( new OOO::Converter, parent, args )
{
}
