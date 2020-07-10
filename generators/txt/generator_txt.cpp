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

#include <KAboutData>
#include <KConfigDialog>
#include <KLocalizedString>

OKULAR_EXPORT_PLUGIN(TxtGenerator, "libokularGenerator_txt.json")

TxtGenerator::TxtGenerator(QObject *parent, const QVariantList &args)
    : Okular::TextDocumentGenerator(new Txt::Converter, QStringLiteral("okular_txt_generator_settings"), parent, args)
{
}

void TxtGenerator::addPages(KConfigDialog *dlg)
{
    Okular::TextDocumentSettingsWidget *widget = new Okular::TextDocumentSettingsWidget();

    dlg->addPage(widget, generalSettings(), i18n("Txt"), QStringLiteral("text-plain"), i18n("Txt Backend Configuration"));
}

#include "generator_txt.moc"
