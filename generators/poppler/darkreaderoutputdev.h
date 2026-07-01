/*
    SPDX-FileCopyrightText: 2026 Okular contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_GENERATOR_PDF_DARKREADEROUTPUTDEV_H
#define OKULAR_GENERATOR_PDF_DARKREADEROUTPUTDEV_H

#include <QImage>
#include <QString>

#include <memory>

class PDFDoc;

/**
 * Renders a PDF page through a custom Poppler SplashOutputDev subclass that,
 * in addition to producing the normal page bitmap, records exactly which
 * device pixels were painted by an image-drawing operator (drawImage,
 * drawImageMask, drawMaskedImage, drawSoftMaskedImage). This provides an
 * exact, draw-call-provenance-based "is this pixel part of an embedded
 * raster image" mask for the Dark Reader feature, replacing the previous
 * per-pixel color heuristic.
 *
 * Both the bitmap and the mask are produced from a SINGLE render pass.
 *
 * This requires driving the render through Poppler's core (non-Qt) API
 * directly: Poppler::Page / Poppler::Document (the Qt wrapper Okular
 * otherwise uses everywhere else in this generator) do not expose the
 * underlying core Page and PDFDoc objects needed to install a custom
 * OutputDev. See DarkReaderRenderer::create() for how a secondary, read-only
 * PDFDoc is opened for this sole purpose, and generator_pdf.cpp for the
 * constraints under which it is used (PDF documents without a password,
 * loaded from a local file path).
 *
 * Only compiled when Okular was built against Poppler's core library
 * headers in addition to poppler-qt6; see HAVE_POPPLER_CORE_OUTPUTDEV.
 */
class DarkReaderRenderer
{
public:
    /**
     * Opens a secondary, independent PDFDoc for @p filePath, used only to
     * drive mask-generating renders for the Dark Reader feature.
     *
     * Returns nullptr if the document could not be opened, or is
     * password-protected. Password-protected documents are deliberately
     * unsupported here: Okular does not retain the plaintext password after
     * unlocking the primary document (to avoid keeping it in memory for the
     * life of the document), so there would be nothing to unlock a second,
     * independent PDFDoc with.
     */
    static std::unique_ptr<DarkReaderRenderer> create(const QString &filePath);

    ~DarkReaderRenderer();

    DarkReaderRenderer(const DarkReaderRenderer &) = delete;
    DarkReaderRenderer &operator=(const DarkReaderRenderer &) = delete;

    /**
     * Renders page @p pageNumber (0-based) at the given resolution, writing
     * the resulting page bitmap to @p outImage (pixel-for-pixel equivalent
     * to what Poppler::Page::renderToImage() would produce for the same
     * page/resolution) and the corresponding "exclude from recoloring" mask
     * to @p outMask (Format_Grayscale8, 255 = image pixel, 0 = text/background
     * pixel).
     *
     * The page is always rendered unrotated (rotate=0); Okular applies view
     * rotation as a separate step of its own for all generators, and the
     * mask must line up with the unrotated bitmap the same way the normal
     * render path does.
     *
     * Returns false if rendering failed for any reason, in which case the
     * caller should fall back to the normal renderToImage() path (and, for
     * the Dark Reader feature specifically, to recoloring the whole page
     * without excluding images).
     */
    bool render(int pageNumber, double dpiX, double dpiY, QImage *outImage, QImage *outMask);

private:
    explicit DarkReaderRenderer(std::unique_ptr<PDFDoc> doc);

    std::unique_ptr<PDFDoc> m_doc;
};

#endif
