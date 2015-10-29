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

#include <KAboutData>
#include <kconfigdialog.h>
#include <KLocalizedString>

static KAboutData createAboutData()
{
  KAboutData aboutData(
                       QStringLiteral("okular_epub"),
                       i18n("EPub Backend"),
                       QStringLiteral("0.2.3"),
                       i18n("An EPub backend"),
                       KAboutLicense::GPL,
                       i18n("© 2008 Ely Levy")
                       );
  aboutData.addAuthor(i18n("Ely Levy"), QString(),
                      QStringLiteral("elylevy@cs.huji.ac.il"));

  return aboutData;
}

OKULAR_EXPORT_PLUGIN( EPubGenerator, createAboutData() )

EPubGenerator::EPubGenerator( QObject *parent, const QVariantList &args )
: Okular::TextDocumentGenerator(new Epub::Converter, QStringLiteral("okular_epub_generator_settings"), parent, args)
{
}

EPubGenerator::~EPubGenerator()
{
}

void EPubGenerator::addPages( KConfigDialog* dlg )
{
  Okular::TextDocumentSettingsWidget *widget = new Okular::TextDocumentSettingsWidget();

  dlg->addPage( widget, generalSettings(), i18n("EPub"), QStringLiteral("application-epub+zip"), i18n("EPub Backend Configuration") );
}

#include "generator_epub.moc"
