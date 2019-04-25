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

#include <QCheckBox>

OKULAR_EXPORT_PLUGIN(MarkdownGenerator, "libokularGenerator_md.json")

bool MarkdownGenerator::s_isFancyPantsEnabled = true;
bool MarkdownGenerator::s_wasFancyPantsEnabled = true;

MarkdownGenerator::MarkdownGenerator( QObject *parent, const QVariantList &args )
    : Okular::TextDocumentGenerator( new Markdown::Converter, QStringLiteral("okular_markdown_generator_settings"), parent, args )
{
    Okular::TextDocumentSettings *mdSettings = generalSettings();

    mdSettings->addItemBool( QStringLiteral("SmartyPants"), s_isFancyPantsEnabled, true );
    mdSettings->load();
    s_wasFancyPantsEnabled = s_isFancyPantsEnabled;
}

bool MarkdownGenerator::reparseConfig()
{
    const bool textDocumentGeneratorChangedConfig = Okular::TextDocumentGenerator::reparseConfig();

    if (s_wasFancyPantsEnabled != s_isFancyPantsEnabled) {
        s_wasFancyPantsEnabled = s_isFancyPantsEnabled;

        Markdown::Converter *c = static_cast<Markdown::Converter*>( converter() );
        c->convertAgain();
        setTextDocument( c->document() );

        return true;
    }

    return textDocumentGeneratorChangedConfig;
}

void MarkdownGenerator::addPages( KConfigDialog* dlg )
{
    Okular::TextDocumentSettingsWidget *widget = new Okular::TextDocumentSettingsWidget();

    QCheckBox *enableSmartyPants = new QCheckBox( dlg );
    enableSmartyPants->setObjectName( QStringLiteral( "kcfg_SmartyPants" ) );
    widget->addRow( i18n("Enable SmartyPants formatting"), enableSmartyPants );

    dlg->addPage( widget, generalSettings(), i18n("Markdown"), QStringLiteral("text-markdown"), i18n("Markdown Backend Configuration") );
}


Q_LOGGING_CATEGORY(OkularMdDebug, "org.kde.okular.generators.md", QtWarningMsg)

#include "generator_md.moc"

