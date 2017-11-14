/***************************************************************************
 *   Copyright (C) 2007 by John Layt <john@layt.net>                       *
 *                                                                         *
 *   FilePrinterPreview based on KPrintPreview (originally LGPL)           *
 *   Copyright (c) 2007 Alex Merry <huntedhacker@tiscali.co.uk>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "fileprinterpreview.h"

#include <QFile>
#include <QSize>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QVBoxLayout>

#include <KWindowConfig>
#include <klocalizedstring.h>
#include <kmimetypetrader.h>
#include <kparts/readonlypart.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <ksharedconfig.h>
#include <QtCore/qloggingcategory.h>

#include "debug_ui.h"

using namespace Okular;

// This code copied from KPrintPreview by Alex Merry, adapted to do PS files instead of PDF

class Okular::FilePrinterPreviewPrivate
{
public:
    FilePrinterPreviewPrivate( FilePrinterPreview *host, const QString & _filename )
        : q(host)
        , mainWidget(new QWidget(host))
        , previewPart(nullptr)
        , failMessage(nullptr)
        , config(KSharedConfig::openConfig(QStringLiteral("digestrc")))

    {
        mainlayout = new QVBoxLayout(q);
        buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, q);
        mainlayout->addWidget(buttonBox);
        filename = _filename;
    }

    void getPart();
    bool doPreview();
    void fail();

    FilePrinterPreview *q;

    QWidget *mainWidget;

    QDialogButtonBox *buttonBox;

    QVBoxLayout *mainlayout;

    QString filename;

    KParts::ReadOnlyPart *previewPart;
    QWidget *failMessage;

    KSharedConfig::Ptr config;
};

void FilePrinterPreviewPrivate::getPart()
{
    if (previewPart) {
        qCDebug(OkularUiDebug) << "already got a part";
        return;
    }
    qCDebug(OkularUiDebug) << "querying trader for application/ps service";

    KPluginFactory *factory(nullptr);
    KService::List offers;
    if (filename.endsWith(QStringLiteral(".ps"))) {
        /* Explicitly look for the Okular/Ghostview part: no other PostScript
           parts are available now; other parts which handles text are not
           suitable here (PostScript source code) */
        offers =
            KMimeTypeTrader::self()->query(QStringLiteral("application/postscript"),
                                           QStringLiteral("KParts/ReadOnlyPart"),
                                           QStringLiteral("[DesktopEntryName] == 'okularghostview'"));
    } else {
        offers = KMimeTypeTrader::self()->query("application/pdf", "KParts/ReadOnlyPart");
    }

    KService::List::ConstIterator it = offers.constBegin();
    while (!factory && it != offers.constEnd()) {
        KPluginLoader loader(**it);
        factory = loader.factory();
        if (!factory) {
            qCDebug(OkularUiDebug) << "Loading failed:" << loader.errorString();
        }
        ++it;
    }
    if (factory) {
        qCDebug(OkularUiDebug) << "Trying to create a part";
        previewPart = factory->create<KParts::ReadOnlyPart>(q, (QVariantList() << QStringLiteral("Print/Preview")));
        if (!previewPart) {
            qCDebug(OkularUiDebug) << "Part creation failed";
        }
    }
}

bool FilePrinterPreviewPrivate::doPreview()
{
    if (!QFile::exists(filename)) {
        qCWarning(OkularUiDebug) << "Nothing was produced to be previewed";
        return false;
    }

    getPart();
    if (!previewPart) {
        //TODO: error dialog
        qCWarning(OkularUiDebug) << "Could not find a PS viewer for the preview dialog";
        fail();
        return false;
    } else {
        mainlayout->insertWidget(0, previewPart->widget());
        return previewPart->openUrl(QUrl::fromLocalFile(filename));
    }
}

void FilePrinterPreviewPrivate::fail()
{
    if (!failMessage) {
        failMessage = new QLabel(i18n("Could not load print preview part"), q);
    }
    mainlayout->insertWidget(0, failMessage);
}




FilePrinterPreview::FilePrinterPreview( const QString &filename, QWidget *parent )
    : QDialog( parent )
    , d( new FilePrinterPreviewPrivate( this, filename ) )
{
    qCDebug(OkularUiDebug) << "kdeprint: creating preview dialog";

    // Set up the dialog
    setWindowTitle(i18n("Print Preview"));

    connect(d->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    KWindowConfig::restoreWindowSize(windowHandle(), d->config->group("Print Preview"));
}

FilePrinterPreview::~FilePrinterPreview()
{
    KConfigGroup group(d->config->group("Print Preview"));
    KWindowConfig::saveWindowSize(windowHandle(), group);

    delete d;
}

QSize FilePrinterPreview::sizeHint() const
{
    // return a more or less useful window size, if not saved already
    return QSize(600, 500);
}

void FilePrinterPreview::showEvent(QShowEvent *event)
{
    if (!event->spontaneous()) {
        // being shown for the first time
        if (!d->doPreview()) {
            event->accept();
            return;
        }
    }
    QDialog::showEvent(event);
}

#include "moc_fileprinterpreview.cpp"
