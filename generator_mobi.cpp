/***************************************************************************
 *   Copyright (C) 2008 by Ely Levy <elylevy@cs.huji.ac.il>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include "generator_mobi.h"

#include "converter.h"

#include <kaboutdata.h>

static KAboutData createAboutData()
{
  KAboutData aboutData(
                       "okular_epub",
                       "okular_epub",
                       ki18n("Mobipocket Backend"),
                       "0.1",
                       ki18n("An mobipocket backend"),
                       KAboutData::License_GPL,
                       ki18n("© 2008 Jakub Stachowski")
                       );
  aboutData.addAuthor(ki18n("Jakub Stachowski"), KLocalizedString(),
                      "qbast@go2.pl");

  return aboutData;
}

OKULAR_EXPORT_PLUGIN( MobiGenerator, createAboutData() )

MobiGenerator::MobiGenerator( QObject *parent, const QVariantList &args )
: Okular::TextDocumentGenerator( new Mobi::Converter, parent, args )
{
}
