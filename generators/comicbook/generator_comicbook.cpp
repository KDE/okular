/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator_comicbook.h"

#include <QPainter>
#include <QPrinter>

#include <KAboutData>
#include <KLocalizedString>

#include <core/document.h>
#include <core/fileprinter.h>
#include <core/page.h>

#include "debug_comicbook.h"

OKULAR_EXPORT_PLUGIN(ComicBookGenerator, "libokularGenerator_comicbook.json")

ComicBookGenerator::ComicBookGenerator(QObject *parent, const QVariantList &args)
    : Generator(parent, args)
{
    setFeature(Threaded);
    setFeature(PrintNative);
    setFeature(PrintToFile);
}

ComicBookGenerator::~ComicBookGenerator()
{
}

bool ComicBookGenerator::loadDocument(const QString &fileName, QVector<Okular::Page *> &pagesVector)
{
    if (!mDocument.open(fileName)) {
        const QString errString = mDocument.lastErrorString();
        if (!errString.isEmpty())
            emit error(errString, -1);
        return false;
    }

    mDocument.pages(&pagesVector);
    return true;
}

bool ComicBookGenerator::doCloseDocument()
{
    mDocument.close();

    return true;
}

QImage ComicBookGenerator::image(Okular::PixmapRequest *request)
{
    int width = request->width();
    int height = request->height();

    QImage image = mDocument.pageImage(request->pageNumber());

    return image.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

bool ComicBookGenerator::print(QPrinter &printer)
{
    QPainter p(&printer);

    QList<int> pageList = Okular::FilePrinter::pageList(printer, document()->pages(), document()->currentPage() + 1, document()->bookmarkedPageList());

    for (int i = 0; i < pageList.count(); ++i) {
        QImage image = mDocument.pageImage(pageList[i] - 1);

        if ((image.width() > printer.width()) || (image.height() > printer.height()))

            image = image.scaled(printer.width(), printer.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

        if (i != 0)
            printer.newPage();

        p.drawImage(0, 0, image);
    }

    return true;
}

#include "generator_comicbook.moc"
