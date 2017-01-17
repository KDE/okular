/***************************************************************************
 *   Copyright (C) 2017 by Gilbert Assaf <gassaf@gmx.de>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator_pdfium.h"

#include <fpdf_doc.h>
#include <fpdf_text.h>
#include <fpdf_sysfontinfo.h>
#include <core/page.h>
#include <QImage>

OKULAR_EXPORT_PLUGIN(PDFiumGenerator, "libokularGenerator_pdfium.json")

PDFiumGenerator::PDFiumGenerator(QObject* parent, const QVariantList& args)
    : Okular::Generator(parent, args)
{
    setFeature(TextExtraction);
}

bool PDFiumGenerator::loadDocument(const QString& fileName, QVector< Okular::Page* >& pagesVector)
{
    FPDF_LIBRARY_CONFIG config;
    config.version = 2;
    config.m_pUserFontPaths = NULL;
    config.m_pIsolate = NULL;
    config.m_v8EmbedderSlot = 0;
    FPDF_InitLibraryWithConfig(&config);

    QByteArray qBA = fileName.toLatin1();
    const char* file_name = qBA.data();
    pdfdoc = FPDF_LoadDocument(file_name, NULL);
    unsigned long err = FPDF_GetLastError();

    if (pdfdoc) {
        if (err == FPDF_ERR_SUCCESS) {
            pagesVector.resize(FPDF_GetPageCount(pdfdoc));

            for (int i = 0; i < FPDF_GetPageCount(pdfdoc); ++i) {

                double width, height;
                FPDF_GetPageSizeByIndex(pdfdoc, i, &width, &height);
                Okular::Page* page = new Okular::Page(i, width, height, Okular::Rotation0);
                page->setLabel(getPageLabel(i));
                pagesVector[ i ] = page;
            }
            return true;
        }
    }

    return false;
}

QImage PDFiumGenerator::image(Okular::PixmapRequest* request)
{
    Okular::Page* page = request->page();

    FPDF_PAGE f_page = FPDF_LoadPage(pdfdoc, page->number());
    int x = request->width();
    int y = request->height();
    QImage image(x, y, QImage::Format_RGBA8888);

    image.fill(0xFFFFFFFF);

    FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(image.width(), image.height(), FPDFBitmap_BGRA, image.scanLine(0), image.bytesPerLine());
    FPDF_RenderPageBitmap(bitmap, f_page, 0, 0, image.width(), image.height(), 0, 0);

    FPDFBitmap_Destroy(bitmap);
    FPDF_ClosePage(f_page);

    return image.rgbSwapped();
}

bool PDFiumGenerator::doCloseDocument()
{
    FPDF_CloseDocument(pdfdoc);
    FPDF_DestroyLibrary();
    return true;
}

Okular::DocumentInfo PDFiumGenerator::generateDocumentInfo(const QSet<Okular::DocumentInfo::Key>& keys) const
{
    Okular::DocumentInfo docInfo;
    docInfo.set(Okular::DocumentInfo::MimeType, QStringLiteral("application/pdf"));

    if (pdfdoc) {
        // compile internal structure reading properties from PDFDoc
        if (keys.contains(Okular::DocumentInfo::Title)) {
            docInfo.set(Okular::DocumentInfo::Title, getMetaText("Title"));
        }
        if (keys.contains(Okular::DocumentInfo::Subject)) {
            docInfo.set(Okular::DocumentInfo::Subject, getMetaText("Subject"));
        }
        if (keys.contains(Okular::DocumentInfo::Author)) {
            docInfo.set(Okular::DocumentInfo::Author, getMetaText("Author"));
        }
        if (keys.contains(Okular::DocumentInfo::Keywords)) {
            docInfo.set(Okular::DocumentInfo::Keywords, getMetaText("Keywords"));
        }
        if (keys.contains(Okular::DocumentInfo::Creator)) {
            docInfo.set(Okular::DocumentInfo::Creator, getMetaText("Creator"));
        }
        if (keys.contains(Okular::DocumentInfo::Producer)) {
            docInfo.set(Okular::DocumentInfo::Producer, getMetaText("Producer"));
        }
        if (keys.contains(Okular::DocumentInfo::CreationDate)) {
            docInfo.set(Okular::DocumentInfo::CreationDate, getMetaText("CreationDate"));
        }
        if (keys.contains(Okular::DocumentInfo::ModificationDate)) {
            docInfo.set(Okular::DocumentInfo::ModificationDate, getMetaText("ModDate"));
        }

        docInfo.set(Okular::DocumentInfo::Pages, QString::number(FPDF_GetPageCount(pdfdoc)));
    }

    return docInfo;
}

Okular::TextPage* PDFiumGenerator::textPage(Okular::Page* page)
{
    Okular::TextPage* tp = new Okular::TextPage;
    FPDF_PAGE f_page = FPDF_LoadPage(pdfdoc, page->number());
    FPDF_TEXTPAGE txt = FPDFText_LoadPage(f_page);
    int allChars = FPDFText_CountChars(txt);
    int numRect = FPDFText_CountRects(txt, 0, allChars);

    for (int i = 0; i < numRect; ++i) {
        unsigned short buffer[500];
        double left, top, right, bottom;
        FPDFText_GetRect(txt, i, &left, &top, &right, &bottom);
        //TODO: suppert larger texts
        FPDFText_GetBoundedText(txt, left, top, right, bottom, buffer, 500);
        top = page->height() - top;
        bottom = page->height() - bottom;

        QPoint topleft(left, top);
        QPoint bottomright(right, bottom);
        QRect rect(bottomright, topleft);

        tp->append(QString::fromUtf16(buffer), new Okular::NormalizedRect(rect, page->width(), page->height()));
    }

    FPDFText_ClosePage(txt);
    FPDF_ClosePage(f_page);

    return tp;
}

QString PDFiumGenerator::getMetaText(const QString& tag) const
{
    char16_t buffer[255];

    //convert to FPDF_BYTESTRING {aka const char*}
    QByteArray ba = tag.toLatin1();
    const char* c_tag = ba.data();

    FPDF_GetMetaText(pdfdoc, c_tag , buffer, 255);
    return QString::fromUtf16(buffer);
}

QString PDFiumGenerator::getPageLabel(int page_index) const
{
    char16_t buffer[255];

    FPDF_GetPageLabel(pdfdoc, page_index , buffer, 255);
    return QString::fromUtf16(buffer);
}


QVariant PDFiumGenerator::metaData(const QString& key, const QVariant& option) const
{
    if (key == QLatin1String("DocumentTitle")) {
        return getMetaText("Title");
    }

    return QVariant();
}

#include "generator_pdfium.moc"
