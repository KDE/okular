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
#include <QtCore/QFile>
#include <QtGui/QLabel>
#include <QtGui/QShowEvent>

#include <kmimetypetrader.h>
#include <kparts/part.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kpushbutton.h>
#include <kservice.h>
#include <kdebug.h>

using namespace Okular;

// This code copied from KPrintPreview by Alex Merry, adapted to do PS files instead of PDF

class Okular::FilePrinterPreviewPrivate
{
public:
    FilePrinterPreviewPrivate( FilePrinterPreview *host, const QString & _filename )
        : q(host)
        , mainWidget(new QWidget(host))
        , previewPart(0)
        , failMessage(0)
        , config(KSharedConfig::openConfig(QString::fromLatin1("okularrc")))

    {
        filename = _filename;
    }

    void getPart();
    bool doPreview();
    void fail();

    FilePrinterPreview *q;

    QWidget *mainWidget;

    QString filename;

    KParts::ReadOnlyPart *previewPart;
    QWidget *failMessage;

    KSharedConfig::Ptr config;
};

void FilePrinterPreviewPrivate::getPart()
{
    if (previewPart) {
        kDebug(500) << "already got a part";
        return;
    }
    kDebug(500) << "querying trader for application/ps service";

    KPluginFactory *factory(0);
    KService::List offers =
        KMimeTypeTrader::self()->query("application/postscript", "KParts/ReadOnlyPart");

    KService::List::ConstIterator it = offers.constBegin();
    while (!factory && it != offers.constEnd()) {
        KPluginLoader loader(**it);
        factory = loader.factory();
        if (!factory) {
            kDebug(500) << "Loading failed:" << loader.errorString();
        }
        ++it;
    }
    if (factory) {
        kDebug(500) << "Trying to create a part";
        previewPart = factory->create<KParts::ReadOnlyPart>(q, (QVariantList() << "Print/Preview"));
        if (!previewPart) {
            kDebug(500) << "Part creation failed";
        }
    }
}

bool FilePrinterPreviewPrivate::doPreview()
{
    if (!QFile::exists(filename)) {
        kWarning() << "Nothing was produced to be previewed";
        return false;
    }

    getPart();
    if (!previewPart) {
        //TODO: error dialog
        kWarning() << "Could not find a PS viewer for the preview dialog";
        fail();
        return false;
    } else {
        q->setMainWidget(previewPart->widget());
        return previewPart->openUrl(filename);
    }
}

void FilePrinterPreviewPrivate::fail()
{
    if (!failMessage) {
        failMessage = new QLabel(i18n("Could not load print preview part"), q);
    }
    q->setMainWidget(failMessage);
}




FilePrinterPreview::FilePrinterPreview( const QString &filename, QWidget *parent )
    : KDialog( parent )
    , d( new FilePrinterPreviewPrivate( this, filename ) )
{
    kDebug(500) << "kdeprint: creating preview dialog";

    // Set up the dialog
    setCaption(i18n("Print Preview"));
    setButtons(KDialog::Close);
    button(KDialog::Close)->setAutoDefault(false);

    restoreDialogSize(d->config->group("Print Preview"));
}

FilePrinterPreview::~FilePrinterPreview()
{
    KConfigGroup group(d->config->group("Print Preview"));
    saveDialogSize(group);

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
    KDialog::showEvent(event);
}

#include "fileprinterpreview.moc"
