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

#include <KLocalizedString>

#include <core/page.h>
#include <core/action.h>
#include <QImage>
#include <QDateTime>

OKULAR_EXPORT_PLUGIN(PDFiumGenerator, "libokularGenerator_pdfium.json")

PDFiumGenerator::PDFiumGenerator(QObject* parent, const QVariantList& args)
    : Okular::Generator(parent, args)
{
    setFeature(TextExtraction);
}

Okular::Document::OpenResult PDFiumGenerator::loadDocumentWithPassword(const QString& filePath, QVector<Okular::Page*>& pagesVector, const QString& password)
{
    FPDF_LIBRARY_CONFIG config;
    config.version = 2;
    config.m_pUserFontPaths = NULL;
    config.m_pIsolate = NULL;
    config.m_v8EmbedderSlot = 0;
    FPDF_InitLibraryWithConfig(&config);

    QByteArray qBFile = filePath.toLatin1();
    const char* file_name = qBFile.data();

    QByteArray qBpwd = password.toUtf8();
    FPDF_BYTESTRING pwd = qBpwd.constData();

    pdfdoc = FPDF_LoadDocument(file_name, pwd);

    unsigned long err = FPDF_GetLastError();

    if (pdfdoc) {
        if (err == FPDF_ERR_SUCCESS) {
            int pageCount = FPDF_GetPageCount(pdfdoc);

            pagesVector.resize(pageCount);
            for (int i = 0; i < pageCount; ++i) {
                double width, height;
                FPDF_GetPageSizeByIndex(pdfdoc, i, &width, &height);
                Okular::Page* page = new Okular::Page(i, width, height, Okular::Rotation0);
                page->setLabel(getPageLabel(i));
                pagesVector[ i ] = page;
            }
            rectsGenerated.fill(false, pageCount);
            return Okular::Document::OpenSuccess;
        } else if (err == FPDF_ERR_PASSWORD) {
            //wrong password
            return Okular::Document::OpenNeedsPassword;
        }
    } else if (err == FPDF_ERR_PASSWORD) {
        //document needs a password, aka const QString& password == ""
        return Okular::Document::OpenNeedsPassword;
    }

    return Okular::Document::OpenError;
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

    // generate links rects only the first time
    bool genObjectRects = !rectsGenerated.at(page->number());

    if (genObjectRects) {
        page->setObjectRects(generateObjectRects(page->number()));
        rectsGenerated[ page->number() ] = true;
    }

    FPDF_ClosePage(f_page);

    return image.rgbSwapped();
}


Okular::Action* PDFiumGenerator::createAction(FPDF_ACTION action, unsigned long type)
{
    Okular::Action* okularAction = 0;
    if (type == PDFACTION_GOTO) {
        FPDF_DEST dest = FPDFAction_GetDest(pdfdoc, action);
        unsigned long page_index = FPDFDest_GetPageIndex(pdfdoc, dest);
        double pageWidth, pageHeight;
        FPDF_GetPageSizeByIndex(pdfdoc, page_index, &pageWidth, &pageHeight);
        FPDF_BOOL hasXCoord, hasYCoord, hasZoom;
        float x, y, zoom;
        FPDFDest_GetLocationInPage(dest, &hasXCoord, &hasYCoord, &hasZoom, &x, &y, &zoom);
        Okular::DocumentViewport viewport;
        viewport.pageNumber = page_index;
        viewport.rePos.normalizedX = x / pageWidth;
        viewport.rePos.normalizedY = (y - pageHeight) / pageHeight;
        viewport.rePos.enabled = true;
        viewport.rePos.pos = Okular::DocumentViewport::TopLeft;

        okularAction = new Okular::GotoAction(QString(), viewport);


    } else if (type == PDFACTION_REMOTEGOTO) {
        char16_t buffer[255];
        FPDFAction_GetFilePath(action, buffer, 255);
        QString fileName = QString::fromUtf16(buffer);

        okularAction = new Okular::GotoAction(fileName, fileName);

    } else if (type == PDFACTION_URI) {
        char buffer[255];
        FPDFAction_GetURIPath(pdfdoc, action, buffer, 255);
        QString url = QString::fromUtf8(buffer);

        okularAction = new Okular::BrowseAction(QUrl(url));

    } else if (type == PDFACTION_LAUNCH) {
        char16_t buffer[255];
        FPDFAction_GetFilePath(action, buffer, 255);
        QString fileName = QString::fromUtf16(buffer);

        okularAction = new Okular::ExecuteAction(fileName, QString());
    }

    return okularAction;
}

QLinkedList<Okular::ObjectRect*> PDFiumGenerator::generateObjectRects(int page_index)
{
    QLinkedList<Okular::ObjectRect*> objectRects;

    FPDF_PAGE f_page = FPDF_LoadPage(pdfdoc, page_index);
    int startPos = 0;
    FPDF_LINK link;
    double pageWidth, pageHeight;
    FPDF_GetPageSizeByIndex(pdfdoc, page_index, &pageWidth, &pageHeight);

    while (FPDFLink_Enumerate(f_page, &startPos, &link)) {
        FPDF_ACTION action = FPDFLink_GetAction(link);
        unsigned long type = FPDFAction_GetType(action);

        if (type == PDFACTION_UNSUPPORTED)
            continue;

        FS_RECTF rect;
        FPDFLink_GetAnnotRect(link, &rect);
        Okular::NormalizedRect rectN = generateRectangle(rect.right, rect.top, rect.left, rect.bottom, pageWidth, pageHeight);
        Okular::ObjectRect* objRect = new Okular::ObjectRect(rectN, false, Okular::ObjectRect::Action, createAction(action, type));
        objectRects.push_front(objRect);
    }

    return objectRects;
}

bool PDFiumGenerator::doCloseDocument()
{
    FPDF_CloseDocument(pdfdoc);
    FPDF_DestroyLibrary();
    docSyn.clear();
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
            QString date = getMetaText("CreationDate");
            date.chop(7); //TODO: support TimeZones
            QDateTime dateTime = QDateTime ::fromString(date, "'D:'yyyyMMddHHmmss");
            docInfo.set(Okular::DocumentInfo::CreationDate, QLocale().toString(dateTime, QLocale::LongFormat));
        }
        if (keys.contains(Okular::DocumentInfo::ModificationDate)) {
            QString date = getMetaText("ModDate");
            date.chop(7); //TODO: support TimeZones
            QDateTime dateTime = QDateTime ::fromString(date, "'D:'yyyyMMddHHmmss");
            docInfo.set(Okular::DocumentInfo::ModificationDate, QLocale().toString(dateTime, QLocale::LongFormat));
        }
        if (keys.contains(Okular::DocumentInfo::CustomKeys)) {
            int fileVersion;
            FPDF_GetFileVersion(pdfdoc, &fileVersion);
            float majorMinor = fileVersion;
            majorMinor = majorMinor / 10; //<Major><Minor> => <Major>.<Minor>
            docInfo.set(QStringLiteral("format"), i18nc("PDF v. <version>", "PDF v. %1",majorMinor), i18n("Format"));
        }

        docInfo.set(Okular::DocumentInfo::Pages, QString::number(FPDF_GetPageCount(pdfdoc)));
    }

    return docInfo;
}

Okular::TextPage* PDFiumGenerator::textPage(Okular::Page* page)
{
    Okular::TextPage* tp = new Okular::TextPage;
    FPDF_PAGE f_page = FPDF_LoadPage(pdfdoc, page->number());
    FPDF_TEXTPAGE text_page = FPDFText_LoadPage(f_page);
    int allChars = FPDFText_CountChars(text_page);

    for (int i = 0; i < allChars; ++i) {
        unsigned int character = FPDFText_GetUnicode(text_page, i);
        QChar c(character);
        QString s(c);
        double left, top, right, bottom;

        FPDFText_GetCharBox(text_page, i, &left, &right, &bottom, &top);

        Okular::NormalizedRect rectN = generateRectangle(left, top, right, bottom, page->width(), page->height());
        tp->append(s, new Okular::NormalizedRect(rectN));
    }

    FPDFText_ClosePage(text_page);
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

Okular::NormalizedRect PDFiumGenerator::generateRectangle(double x1, double y1, double x2, double y2, double pageWidth, double pageHeight)
{
    //convert bottomright to upright coordinate system
    y1 = pageHeight - y1;
    y2 = pageHeight - y2;

    QPoint topleft(x1, y1);
    QPoint bottomright(x2, y2);
    QRect rect(bottomright, topleft);
    return Okular::NormalizedRect(rect, pageWidth, pageHeight);
}


QVariant PDFiumGenerator::metaData(const QString& key, const QVariant& option) const
{
    if (key == QLatin1String("DocumentTitle")) {
        return getMetaText("Title");
    }

    return QVariant();
}

const Okular::DocumentSynopsis* PDFiumGenerator::generateDocumentSynopsis()
{
    if (!pdfdoc)
        return NULL;

    FPDF_BOOKMARK bookmark = FPDFBookmark_GetFirstChild(pdfdoc, NULL);

    addSynopsisChildren(bookmark, &docSyn);
    return &docSyn;
}

void PDFiumGenerator::addSynopsisChildren(FPDF_BOOKMARK bookmark, QDomNode* parentDestination)
{
    while (bookmark != NULL) {
        char16_t buffer_new[255];
        FPDFBookmark_GetTitle(bookmark, buffer_new, 255);
        QString title = QString::fromUtf16(buffer_new);
        QDomElement item = docSyn.createElement(title);
        parentDestination->appendChild(item);

        FPDF_ACTION action = FPDFBookmark_GetAction(bookmark);
        unsigned long type = FPDFAction_GetType(action);

        if (type == PDFACTION_GOTO) {
            Okular::Action* oAction = createAction(action, type);
            Okular::GotoAction* gotoAction = static_cast<Okular::GotoAction*>(oAction);
            item.setAttribute(QStringLiteral("Viewport"), gotoAction->destViewport().toString());
        } else if (type == PDFACTION_REMOTEGOTO) {
            Okular::Action* oAction = createAction(action, type);
            Okular::GotoAction* gotoAction = static_cast<Okular::GotoAction*>(oAction);

            item.setAttribute(QStringLiteral("ExternalFileName"), gotoAction->fileName());

        } else if (type == PDFACTION_URI) {
            Okular::Action* oAction = createAction(action, type);
            Okular::BrowseAction* browseAction = static_cast<Okular::BrowseAction*>(oAction);

            item.setAttribute(QStringLiteral("URL"), browseAction->url().toString());

        }

        FPDF_BOOKMARK sibling = FPDFBookmark_GetFirstChild(pdfdoc, bookmark);
        addSynopsisChildren(sibling, &item);
        bookmark = FPDFBookmark_GetNextSibling(pdfdoc, bookmark);
    }
}

#include "generator_pdfium.moc"
