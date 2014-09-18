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
#include <klocale.h>
#include <kconfigdialog.h>
#include <kwallet.h>

static KAboutData createAboutData()
{
    KAboutData aboutData(
         QStringLiteral("okular_ooo"),
         i18n( "OpenDocument Text Backend" ),
         QStringLiteral("0.2.4"),
         i18n( "A renderer for OpenDocument Text documents" ),
         KAboutLicense::GPL,
         i18n( "© 2006-2008 Tobias Koenig" )
    );
    aboutData.addAuthor( QStringLiteral( "Tobias Koenig" ), QString(), QStringLiteral("tokoe@kde.org") );

    return aboutData;
}

OKULAR_EXPORT_PLUGIN( KOOOGenerator, createAboutData() )

KOOOGenerator::KOOOGenerator( QObject *parent, const QVariantList &args )
  : Okular::TextDocumentGenerator( new OOO::Converter, "okular_ooo_generator_settings", parent, args )
{
}

void KOOOGenerator::addPages( KConfigDialog* dlg )
{
    Okular::TextDocumentSettingsWidget *widget = new Okular::TextDocumentSettingsWidget();

    dlg->addPage( widget, generalSettings(), i18n("OpenDocument Text"), "application-vnd.oasis.opendocument.text", i18n("OpenDocument Text Backend Configuration") );
}

void KOOOGenerator::walletDataForFile( const QString &fileName, QString *walletName, QString *walletFolder, QString *walletKey ) const
{
    *walletKey = fileName + "/opendocument";
    *walletName = KWallet::Wallet::LocalWallet();
    *walletFolder = KWallet::Wallet::PasswordFolder();
}
