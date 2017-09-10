/***************************************************************************
 *   Copyright (C) 2017 by Julian Wolff <wolff@julianwolff.de>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator_md.h"

#include "converter.h"
#include "debug_md.h"

#include <KAboutData>
#include <KLocalizedString>
#include <kconfigdialog.h>

OKULAR_EXPORT_PLUGIN(MarkdownGenerator, "libokularGenerator_md.json")

MarkdownGenerator::MarkdownGenerator( QObject *parent, const QVariantList &args )
    : Okular::TextDocumentGenerator( new Markdown::Converter, QStringLiteral("okular_markdown_generator_settings"), parent, args )
{
}

void MarkdownGenerator::addPages( KConfigDialog* dlg )
{
    Okular::TextDocumentSettingsWidget *widget = new Okular::TextDocumentSettingsWidget();

    dlg->addPage( widget, generalSettings(), i18n("Markdown"), QStringLiteral("text-markdown"), i18n("Markdown Backend Configuration") );
}


Q_LOGGING_CATEGORY(OkularMdDebug, "org.kde.okular.generators.md", QtWarningMsg)

#include "generator_md.moc"

