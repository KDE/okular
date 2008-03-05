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
#include <klocale.h>

static KAboutData createAboutData()
{
    KAboutData aboutData(
         "okular_ooo",
         "okular_ooo",
         ki18n( "OpenOffice Document Backend" ),
         "0.1",
         ki18n( "A renderer for OpenOffice text documents" ),
         KAboutData::License_GPL,
         ki18n( "© 2006-2008 Tobias Koenig" )
    );
    aboutData.addAuthor( ki18n( "Tobias Koenig" ), KLocalizedString(), "tokoe@kde.org" );

    return aboutData;
}

OKULAR_EXPORT_PLUGIN( KOOOGenerator, createAboutData() )

KOOOGenerator::KOOOGenerator( QObject *parent, const QVariantList &args )
  : Okular::TextDocumentGenerator( new OOO::Converter, parent, args )
{
}
