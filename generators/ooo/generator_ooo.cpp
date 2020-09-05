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

#include <KAboutData>
#include <KConfigDialog>
#include <KLocalizedString>
#include <kwallet.h>

OKULAR_EXPORT_PLUGIN(KOOOGenerator, "libokularGenerator_ooo.json")

KOOOGenerator::KOOOGenerator(QObject *parent, const QVariantList &args)
    : Okular::TextDocumentGenerator(new OOO::Converter, QStringLiteral("okular_ooo_generator_settings"), parent, args)
{
}

void KOOOGenerator::addPages(KConfigDialog *dlg)
{
    Okular::TextDocumentSettingsWidget *widget = new Okular::TextDocumentSettingsWidget();

    dlg->addPage(widget, generalSettings(), i18n("OpenDocument Text"), QStringLiteral("application-vnd.oasis.opendocument.text"), i18n("OpenDocument Text Backend Configuration"));
}

void KOOOGenerator::walletDataForFile(const QString &fileName, QString *walletName, QString *walletFolder, QString *walletKey) const
{
    *walletKey = fileName + QStringLiteral("/opendocument");
    *walletName = KWallet::Wallet::LocalWallet();
    *walletFolder = KWallet::Wallet::PasswordFolder();
}
#include "generator_ooo.moc"
