/*
    SPDX-FileCopyrightText: 2004-2008 Albert Astals Cid <aacid@kde.org>
    SPDX-FileCopyrightText: 2004 Enrico Ros <eros.kde@email.it>

    Work sponsored by the LiMux project of the city of Munich:
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company <info@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_GENERATOR_PDF_H_
#define _OKULAR_GENERATOR_PDF_H_

// #include "synctex/synctex_parser.h"

#include <poppler-qt6.h>
#include <poppler-version.h>

#include <QBitArray>
#include <QPointer>
#include <QSet>

#include <core/annotations.h>
#include <core/document.h>
#include <core/generator.h>
#include <core/printoptionswidget.h>
#include <interfaces/configinterface.h>
#include <interfaces/printinterface.h>
#include <interfaces/saveinterface.h>

#include <unordered_map>

#if HAVE_POPPLER_CORE_OUTPUTDEV
#include "darkreaderoutputdev.h"
#endif

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
    Okular::Document::OpenResult loadDocumentWithPassword(const QString &filePath, QList<Okular::Page *> &pagesVector, const QString &password) override;
    Okular::Document::OpenResult loadDocumentFromDataWithPassword(const QByteArray &fileData, QList<Okular::Page *> &pagesVector, const QString &password) override;
    void loadPages(QList<Okular::Page *> &pagesVector, int rotation = -1, bool clear = false);
    // [INHERITED] document information
    Okular::DocumentInfo generateDocumentInfo(const QSet<Okular::DocumentInfo::Key> &keys) const override;
    const Okular::DocumentSynopsis *generateDocumentSynopsis() override;
    Okular::FontInfo::List fontsForPage(int page) override;
    const QList<Okular::EmbeddedFile *> *embeddedFiles() const override;
    PageLayout defaultPageLayout() const override;
    bool defaultPageContinuous() const override;
    PageSizeMetric pagesSizeMetric() const override
    {
        return Pixels;
    }
    QAbstractItemModel *layersModel() const override;
    Okular::BackendOpaqueAction::OpaqueActionResult opaqueAction(const Okular::BackendOpaqueAction *action) override;

    // [INHERITED] document information
    bool isAllowed(Okular::Permission permission) const override;

    // [INHERITED] perform actions on document / pages
    QImage image(Okular::PixmapRequest *request) override;

    // [INHERITED] print page using an already configured kprinter
    Okular::Document::PrintError print(QPrinter &printer) override;

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

    bool canSign() const override;
    std::pair<Okular::SigningResult, QString> sign(const Okular::NewSignatureData &oData, const QString &rFilename) override;

    Okular::CertificateStore *certificateStore() const override;

    QByteArray requestFontData(const Okular::FontInfo &font) override;

    static void okularToPoppler(const Okular::NewSignatureData &oData, Poppler::PDFConverter::NewSignatureData *pData);

    enum DocumentAdditionalActionType {
        CloseDocument,
        SaveDocumentStart,
        SaveDocumentFinish,
        PrintDocumentStart,
        PrintDocumentFinish,
    };
    Okular::Action *additionalDocumentAction(Okular::Document::DocumentAdditionalActionType type) override;

protected:
    SwapBackingFileResult swapBackingFile(QString const &newFileName, QList<Okular::Page *> &newPagesVector) override;
    bool doCloseDocument() override;
    Okular::TextPage *textPage(Okular::TextRequest *request) override;

private:
    Okular::Document::OpenResult init(QList<Okular::Page *> &pagesVector, const QString &password);

    // create the document synopsis hierarchy
    void addSynopsisChildren(const QList<Poppler::OutlineItem> &outlineItems, QDomNode *parentDestination);
    // fetch annotations from the pdf file and add they to the page
    void addAnnotations(Poppler::Page *popplerPage, Okular::Page *page);
    // fetch the transition information and add it to the page
    void addTransition(Poppler::Page *pdfPage, Okular::Page *page);
    // fetch the poppler page form fields
    QList<Okular::FormField *> getFormFields(Poppler::Page *popplerPage);

    Okular::TextPage *abstractTextPage(const std::vector<std::unique_ptr<Poppler::TextBox>> &text, double height, double width, int rot);

    void resolveMediaLinkReferences(Okular::Page *page);
    void resolveMediaLinkReference(Okular::Action *action);

    bool setDocumentRenderHints();

    // poppler dependent stuff
    std::unique_ptr<Poppler::Document> pdfdoc;

    void xrefReconstructionHandler();

    // misc variables for document info and synopsis caching
    QString documentFilePath;
    bool docSynopsisDirty;
    bool xrefReconstructed;
    bool hasVisibleOverprint;
    Okular::DocumentSynopsis docSyn;
    mutable bool docEmbeddedFilesDirty;
    mutable QList<Okular::EmbeddedFile *> docEmbeddedFiles;
    int nextFontPage;
    PopplerAnnotationProxy *annotProxy;
    mutable std::unique_ptr<Okular::CertificateStore> certStore;
    // the hash below only contains annotations that were present on the file at open time
    // this is enough for what we use it for
    QHash<Okular::Annotation *, Poppler::Annotation *> annotationsOnOpenHash;

    QBitArray rectsGenerated;

    QPointer<PDFOptionsPage> pdfOptionsPage;

    bool documentHasPassword = false;
    QHash<int, Okular::Action *> m_additionalDocumentActions;
    void setAdditionalDocumentAction(Okular::Document::DocumentAdditionalActionType type, Okular::Action *action);

#if HAVE_POPPLER_CORE_OUTPUTDEV
    // BEGIN Dark Reader exact image mask
    // Renders a page via a custom Poppler core OutputDev that additionally
    // records exactly which pixels belong to embedded raster images, so
    // that PagePainter's Dark Reader recoloring can leave them untouched.
    // Only attempted for PDFs opened from a local file path, with no
    // password (see darkreaderoutputdev.h for why). Returns false if the
    // mask-aware render could not be produced for any reason; callers must
    // fall back to the normal renderToImage() path in that case.
    bool renderWithDarkReaderMask(Okular::PixmapRequest *request, double dpiX, double dpiY, QImage *outImage);

    // Makes sure m_darkReaderMaskCache has a valid mask for @p page at the
    // given resolution, generating one via DarkReaderRenderer if needed —
    // regardless of whether the pixmap request that triggered this call is
    // itself eligible to reuse the accompanying bitmap (tile/partial requests
    // are not, but should still be able to prime the mask cache). No-op if a
    // matching mask is already cached.
    void ensureDarkReaderMask(const Okular::Page *page, double dpiX, double dpiY);

    // Secondary, independent PDFDoc used only to drive Dark Reader's
    // mask-generating renders (see darkreaderoutputdev.h for why a second
    // document is needed at all). Created lazily on first use and kept
    // around for as long as it keeps succeeding; reset whenever it fails so
    // the next attempt can retry from scratch (e.g. if the file was briefly
    // unavailable).
    std::unique_ptr<DarkReaderRenderer> m_darkReaderRenderer;

    // Most recently generated "exclude from recoloring" mask per page
    // number, in the same (unrotated) orientation as the page bitmap. Reset
    // whenever the document is closed. Deliberately not resized/rotated
    // proactively: PagePainter adapts it to the current view geometry when
    // it is consumed, the same way it already adapts the cached pixmap.
    struct DarkReaderMaskEntry {
        QImage mask;
        double dpiX = 0;
        double dpiY = 0;
    };
    QHash<int, DarkReaderMaskEntry> m_darkReaderMaskCache;
    QSet<int> m_pagesWithUnsavedAnnotations;
    // END Dark Reader exact image mask
#endif
};

#endif

/* kate: replace-tabs on; indent-width 4; */
