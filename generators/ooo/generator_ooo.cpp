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

OKULAR_EXPORT_PLUGIN(KOOOGenerator)

KOOOGenerator::KOOOGenerator()
  : Okular::TextDocumentGenerator( new OOO::Converter )
{
    // ### TODO fill after the KDE 4.0 unfreeze
    KAboutData *about = new KAboutData(
         "okular_ooo",
         "okular_ooo",
         KLocalizedString(),
         "0.1",
         KLocalizedString(),
         KAboutData::License_GPL,
         KLocalizedString()
    );
    setAboutData( about );
}
