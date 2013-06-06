/***************************************************************************
 *   Copyright (C) 2013 by Azat Khuzhin <a3at.mail@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/


#include "generator_txt.h"
#include "converter.h"

#include <kaboutdata.h>
#include <KConfigDialog>

static KAboutData createAboutData()
{
    KAboutData aboutData(
         "okular_txt",
         "okular_txt",
         ki18n( "Txt Backend" ),
         "0.1",
         ki18n( "Txt backend." ),
         KAboutData::License_GPL,
         ki18n( "© 2013 Azat Khuzhin" )
    );
    aboutData.addAuthor( ki18n( "Azat Khuzhin" ), KLocalizedString(), "a3at.mail@gmail.com" );
    return aboutData;
}

OKULAR_EXPORT_PLUGIN( TxtGenerator, createAboutData() )

TxtGenerator::TxtGenerator( QObject *parent, const QVariantList &args )
    : Okular::TextDocumentGenerator( new Txt::Converter, "okular_txt_generator_settings", parent, args )
{
}

void TxtGenerator::addPages( KConfigDialog* dlg )
{
    Okular::TextDocumentSettingsWidget *widget = new Okular::TextDocumentSettingsWidget();

    dlg->addPage( widget, generalSettings(), i18n("Txt"), "text-plain", i18n("Txt Backend Configuration") );
}
