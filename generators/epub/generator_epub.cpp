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
#include <KConfigDialog>
#include <KLocalizedString>

OKULAR_EXPORT_PLUGIN(EPubGenerator, "libokularGenerator_epub.json")

EPubGenerator::EPubGenerator(QObject *parent, const QVariantList &args)
    : Okular::TextDocumentGenerator(new Epub::Converter, QStringLiteral("okular_epub_generator_settings"), parent, args)
{
}

EPubGenerator::~EPubGenerator()
{
}

void EPubGenerator::addPages(KConfigDialog *dlg)
{
    Okular::TextDocumentSettingsWidget *widget = new Okular::TextDocumentSettingsWidget();

    dlg->addPage(widget, generalSettings(), i18n("EPub"), QStringLiteral("application-epub+zip"), i18n("EPub Backend Configuration"));
}

#include "generator_epub.moc"
