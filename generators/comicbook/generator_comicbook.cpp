/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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

bool ComicBookGenerator::loadDocument(const QString &fileName, QList<Okular::Page *> &pagesVector)
{
    if (!mDocument.open(fileName)) {
        const QString errString = mDocument.lastErrorString();
        if (!errString.isEmpty()) {
            Q_EMIT error(errString, -1);
        }
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

    QImage pageImage = mDocument.pageImage(request->pageNumber());

    return pageImage.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

Okular::Document::PrintError ComicBookGenerator::print(QPrinter &printer)
{
    QPainter p(&printer);

    QList<int> pageList = Okular::FilePrinter::pageList(printer, document()->pages(), document()->currentPage() + 1, document()->bookmarkedPageList());

    for (int i = 0; i < pageList.count(); ++i) {
        QImage pageImage = mDocument.pageImage(pageList[i] - 1);

        if ((pageImage.width() > printer.width()) || (pageImage.height() > printer.height())) {
            pageImage = pageImage.scaled(printer.width(), printer.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        if (i != 0) {
            printer.newPage();
        }

        p.drawImage(0, 0, pageImage);
    }

    return Okular::Document::NoPrintError;
}

#include "generator_comicbook.moc"
