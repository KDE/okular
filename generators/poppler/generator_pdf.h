/***************************************************************************
 *   Copyright (C) 2004-2008 by Albert Astals Cid <aacid@kde.org>          *
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2017    Klarälvdalens Datakonsult AB, a KDAB Group      *
 *                         company, info@kdab.com. Work sponsored by the   *
 *                         LiMux project of the city of Munich             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_PDF_H_
#define _OKULAR_GENERATOR_PDF_H_

//#include "synctex/synctex_parser.h"
#include <config-okular-poppler.h>

#include <poppler-qt5.h>

#include <QBitArray>
#include <QPointer>

#include <core/document.h>
#include <core/generator.h>
#include <core/printoptionswidget.h>
#include <interfaces/configinterface.h>
#include <interfaces/printinterface.h>
#include <interfaces/saveinterface.h>

class PDFOptionsPage;
class PopplerAnnotationProxy;

/**
 * @short A generator that builds contents from a PDF document.
 *
 * All Generator features are supported and implemented by this one.
 * Internally this holds a reference to xpdf's core objects and provides
 * contents generation using the PDFDoc object and a couple of OutputDevices
 * called Okular::OutputDev and Okular::TextDev (both defined in gp_outputdev.h).
 *
 * For generating page contents we tell PDFDoc to render a page and grab
 * contents from out OutputDevs when rendering finishes.
 *
 */
class PDFGenerator : public Okular::Generator, public Okular::ConfigInterface, public Okular::PrintInterface, public Okular::SaveInterface
{
    Q_OBJECT
    Q_INTERFACES(Okular::Generator)
    Q_INTERFACES(Okular::ConfigInterface)
    Q_INTERFACES(Okular::PrintInterface)
    Q_INTERFACES(Okular::SaveInterface)

public:
    PDFGenerator(QObject *parent, const QVariantList &args);
    ~PDFGenerator() override;

    // [INHERITED] load a document and fill up the pagesVector
    Okular::Document::OpenResult loadDocumentWithPassword(const QString &filePath, QVector<Okular::Page *> &pagesVector, const QString &password) override;
    Okular::Document::OpenResult loadDocumentFromDataWithPassword(const QByteArray &fileData, QVector<Okular::Page *> &pagesVector, const QString &password) override;
    void loadPages(QVector<Okular::Page *> &pagesVector, int rotation = -1, bool clear = false);
    // [INHERITED] document information
    Okular::DocumentInfo generateDocumentInfo(const QSet<Okular::DocumentInfo::Key> &keys) const override;
    const Okular::DocumentSynopsis *generateDocumentSynopsis() override;
    Okular::FontInfo::List fontsForPage(int page) override;
    const QList<Okular::EmbeddedFile *> *embeddedFiles() const override;
    PageSizeMetric pagesSizeMetric() const override
    {
        return Pixels;
    }
    QAbstractItemModel *layersModel() const override;
    void opaqueAction(const Okular::BackendOpaqueAction *action) override;

    // [INHERITED] document information
    bool isAllowed(Okular::Permission permission) const override;

    // [INHERITED] perform actions on document / pages
    QImage image(Okular::PixmapRequest *request) override;

    // [INHERITED] print page using an already configured kprinter
    bool print(QPrinter &printer) override;

    // [INHERITED] reply to some metadata requests
    QVariant metaData(const QString &key, const QVariant &option) const override;

    // [INHERITED] reparse configuration
    bool reparseConfig() override;
    void addPages(KConfigDialog *) override;

    // [INHERITED] text exporting
    Okular::ExportFormat::List exportFormats() const override;
    bool exportTo(const QString &fileName, const Okular::ExportFormat &format) override;

    // [INHERITED] print interface
    Okular::PrintOptionsWidget *printConfigurationWidget() const override;

    // [INHERITED] save interface
    bool supportsOption(SaveOption) const override;
    bool save(const QString &fileName, SaveOptions options, QString *errorText) override;
    Okular::AnnotationProxy *annotationProxy() const override;

protected:
    SwapBackingFileResult swapBackingFile(QString const &newFileName, QVector<Okular::Page *> &newPagesVector) override;
    bool doCloseDocument() override;
    Okular::TextPage *textPage(Okular::TextRequest *request) override;
    Q_INVOKABLE Okular::Generator::PrintError printError() const;

protected Q_SLOTS:
    void requestFontData(const Okular::FontInfo &font, QByteArray *data);

private:
    Okular::Document::OpenResult init(QVector<Okular::Page *> &pagesVector, const QString &password);

    // create the document synopsis hierarchy
#ifdef HAVE_POPPLER_0_74
    void addSynopsisChildren(const QVector<Poppler::OutlineItem> &outlineItems, QDomNode *parentDestination);
#else
    void addSynopsisChildren(QDomNode *parentSource, QDomNode *parentDestination);
#endif
    // fetch annotations from the pdf file and add they to the page
    void addAnnotations(Poppler::Page *popplerPage, Okular::Page *page);
    // fetch the transition information and add it to the page
    void addTransition(Poppler::Page *pdfPage, Okular::Page *page);
    // fetch the poppler page form fields
    QLinkedList<Okular::FormField *> getFormFields(Poppler::Page *popplerPage);

    Okular::TextPage *abstractTextPage(const QList<Poppler::TextBox *> &text, double height, double width, int rot);

    void resolveMediaLinkReferences(Okular::Page *page);
    void resolveMediaLinkReference(Okular::Action *action);

    bool setDocumentRenderHints();

    // poppler dependent stuff
    Poppler::Document *pdfdoc;

    // misc variables for document info and synopsis caching
    bool docSynopsisDirty;
    Okular::DocumentSynopsis docSyn;
    mutable bool docEmbeddedFilesDirty;
    mutable QList<Okular::EmbeddedFile *> docEmbeddedFiles;
    int nextFontPage;
    PopplerAnnotationProxy *annotProxy;
    // the hash below only contains annotations that were present on the file at open time
    // this is enough for what we use it for
    QHash<Okular::Annotation *, Poppler::Annotation *> annotationsOnOpenHash;

    QBitArray rectsGenerated;

    QPointer<PDFOptionsPage> pdfOptionsPage;

    PrintError lastPrintError;
};

#endif

/* kate: replace-tabs on; indent-width 4; */
