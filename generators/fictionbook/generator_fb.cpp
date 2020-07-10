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

#include <KAboutData>
#include <KConfigDialog>
#include <KLocalizedString>

OKULAR_EXPORT_PLUGIN(FictionBookGenerator, "libokularGenerator_fb.json")

FictionBookGenerator::FictionBookGenerator(QObject *parent, const QVariantList &args)
    : Okular::TextDocumentGenerator(new FictionBook::Converter, QStringLiteral("okular_fictionbook_generator_settings"), parent, args)
{
}

void FictionBookGenerator::addPages(KConfigDialog *dlg)
{
    Okular::TextDocumentSettingsWidget *widget = new Okular::TextDocumentSettingsWidget();

    dlg->addPage(widget, generalSettings(), i18n("FictionBook"), QStringLiteral("okular-fb2"), i18n("FictionBook Backend Configuration"));
}
#include "generator_fb.moc"
