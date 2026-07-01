/*
    SPDX-FileCopyrightText: 2026 Okular contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "darkreaderoutputdev.h"

#include <GfxState.h>
#include <GlobalParams.h>
#include <Object.h>
#include <PDFDoc.h>
#include <SplashOutputDev.h>
#include <Stream.h>
#include <goo/GooString.h>
#include <splash/SplashBitmap.h>

#include <QSysInfo>

#include <algorithm>
#include <cmath>

namespace
{

/**
 * A SplashOutputDev that, alongside normal Splash rendering, records the
 * device-pixel bounding box of every image-drawing operation into a
 * grayscale mask.
 *
 * Note that setSoftMaskFromImageMask()/unsetSoftMaskFromImageMask() are
 * deliberately NOT overridden here: those calls only install/clear a soft
 * mask used to clip *subsequent* fill operations (a common technique for
 * drawing anti-aliased or gradient-clipped vector shapes), they do not by
 * themselves paint an image. Pixels painted while such a soft mask is
 * active go through Splash's normal fill/stroke path, not through one of
 * the drawImage* overrides below, so they are correctly left out of the
 * mask and remain eligible for recoloring like any other vector content.
 *
 * Nested Form XObjects (images inside forms inside forms) need no special
 * handling: Poppler's own content-stream interpreter (Gfx::doForm())
 * recurses into forms and invokes these same overridden methods for
 * whatever they draw, regardless of nesting depth.
 */
class MaskingSplashOutputDev : public SplashOutputDev
{
public:
    MaskingSplashOutputDev(SplashColorMode colorModeA, int bitmapRowPadA, bool reverseVideoA, SplashColorPtr paperColorA, bool bitmapTopDownA, SplashThinLineMode thinLineModeA)
        : SplashOutputDev(colorModeA, bitmapRowPadA, reverseVideoA, paperColorA, bitmapTopDownA, thinLineModeA)
    {
    }

    void startPage(int pageNum, GfxState *state, XRef *xrefA) override
    {
        SplashOutputDev::startPage(pageNum, state, xrefA);

        // The bitmap size is only known once the base class has set up the
        // page, so the mask is (re)created here rather than in the
        // constructor.
        maskImage = QImage(getBitmapWidth(), getBitmapHeight(), QImage::Format_Grayscale8);
        maskImage.fill(0);
    }

    void drawImageMask(GfxState *state, Object *ref, Stream *str, int width, int height, bool invert, bool interpolate, bool inlineImg) override
    {
        SplashOutputDev::drawImageMask(state, ref, str, width, height, invert, interpolate, inlineImg);
        markImageRegion(state);
    }

    void drawImage(GfxState *state, Object *ref, Stream *str, int width, int height, GfxImageColorMap *colorMap, bool interpolate, const int *maskColors, bool inlineImg) override
    {
        SplashOutputDev::drawImage(state, ref, str, width, height, colorMap, interpolate, maskColors, inlineImg);
        markImageRegion(state);
    }

    void drawMaskedImage(GfxState *state, Object *ref, Stream *str, int width, int height, GfxImageColorMap *colorMap, bool interpolate, Stream *maskStr, int maskWidth, int maskHeight, bool maskInvert, bool maskInterpolate) override
    {
        SplashOutputDev::drawMaskedImage(state, ref, str, width, height, colorMap, interpolate, maskStr, maskWidth, maskHeight, maskInvert, maskInterpolate);
        markImageRegion(state);
    }

    void drawSoftMaskedImage(GfxState *state,
                             Object *ref,
                             Stream *str,
                             int width,
                             int height,
                             GfxImageColorMap *colorMap,
                             bool interpolate,
                             Stream *maskStr,
                             int maskWidth,
                             int maskHeight,
                             GfxImageColorMap *maskColorMap,
                             bool maskInterpolate) override
    {
        SplashOutputDev::drawSoftMaskedImage(state, ref, str, width, height, colorMap, interpolate, maskStr, maskWidth, maskHeight, maskColorMap, maskInterpolate);
        markImageRegion(state);
    }

    QImage maskImage;

private:
    void markImageRegion(GfxState *state)
    {
        if (maskImage.isNull()) {
            return;
        }

        // Images are always painted into the unit square in user space;
        // map its four corners through the CTM to find the device-pixel
        // bounding box actually covered by this draw call. A bounding box
        // (rather than exact per-pixel coverage, which would require
        // re-deriving Splash's own clipping/sampling) is used deliberately:
        // it is much cheaper to compute, and for the purpose of excluding
        // images from recoloring, slightly over-including a sliver of
        // background around a rotated/skewed image is an acceptable
        // trade-off.
        double xs[4];
        double ys[4];
        state->transform(0, 0, &xs[0], &ys[0]);
        state->transform(1, 0, &xs[1], &ys[1]);
        state->transform(0, 1, &xs[2], &ys[2]);
        state->transform(1, 1, &xs[3], &ys[3]);

        const double xMin = *std::min_element(xs, xs + 4);
        const double xMax = *std::max_element(xs, xs + 4);
        const double yMin = *std::min_element(ys, ys + 4);
        const double yMax = *std::max_element(ys, ys + 4);

        const int ixMin = std::clamp(static_cast<int>(std::floor(xMin)), 0, maskImage.width());
        const int ixMax = std::clamp(static_cast<int>(std::ceil(xMax)), 0, maskImage.width());
        const int iyMin = std::clamp(static_cast<int>(std::floor(yMin)), 0, maskImage.height());
        const int iyMax = std::clamp(static_cast<int>(std::ceil(yMax)), 0, maskImage.height());

        for (int y = iyMin; y < iyMax; ++y) {
            uchar *row = maskImage.scanLine(y);
            std::fill(row + ixMin, row + ixMax, uchar(255));
        }
    }
};

}

std::unique_ptr<DarkReaderRenderer> DarkReaderRenderer::create(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return nullptr;
    }

    // Poppler's core API requires the process-wide globalParams to be
    // initialized. Poppler::Document::load() (used for the primary,
    // poppler-qt6-backed document) already initializes it as a side effect,
    // and that always happens before a PDF can be displayed at all, so by
    // the time Dark Reader can be toggled on for a document, globalParams is
    // guaranteed to already exist. This check is a defensive fallback, not
    // the "normal" way globalParams is expected to get set up.
    if (!globalParams) {
        return nullptr;
    }

    auto doc = std::make_unique<PDFDoc>(std::make_unique<GooString>(filePath.toUtf8().constData()));
    if (!doc->isOk()) {
        return nullptr;
    }
    if (doc->isEncrypted()) {
        // See the class documentation: password-protected documents are
        // deliberately unsupported for this feature.
        return nullptr;
    }

    return std::unique_ptr<DarkReaderRenderer>(new DarkReaderRenderer(std::move(doc)));
}

DarkReaderRenderer::DarkReaderRenderer(std::unique_ptr<PDFDoc> doc)
    : m_doc(std::move(doc))
{
}

DarkReaderRenderer::~DarkReaderRenderer() = default;

bool DarkReaderRenderer::render(int pageNumber, double dpiX, double dpiY, QImage *outImage, QImage *outMask)
{
    if (!m_doc || !outImage || !outMask) {
        return false;
    }
    if (pageNumber < 0 || pageNumber >= m_doc->getNumPages()) {
        return false;
    }

    SplashColor paperColor;
    paperColor[0] = 255;
    paperColor[1] = 255;
    paperColor[2] = 255;

    // splashModeXBGR8 plus the QImage::Format_RGB32 conversion below mirrors
    // exactly what poppler-qt6's Page::renderToImage() does internally for
    // the (default) opaque, non-overprint-preview case (see
    // Qt6SplashOutputDev::getXBGRImage() in poppler's qt6/src/poppler-page.cc),
    // so the bitmap this produces is pixel-for-pixel equivalent to what the
    // normal (non-Dark-Reader) rendering path would have produced for the
    // same page and resolution.
    MaskingSplashOutputDev out(splashModeXBGR8, 4, false, paperColor, true, splashThinLineDefault);
    out.startDoc(m_doc.get());
    out.setFontAntialias(true);
    out.setVectorAntialias(true);

    // Poppler page numbers for displayPageSlice() are 1-based. Rotation is
    // always 0 here: Okular applies view rotation itself, uniformly across
    // all generators, as a separate step performed on the returned bitmap;
    // the mask must be produced in the same unrotated orientation so it
    // still lines up after that step is applied.
    m_doc->displayPageSlice(&out, pageNumber + 1, dpiX, dpiY, 0, false, true, false, -1, -1, -1, -1);

    SplashBitmap *bitmap = out.getBitmap();
    if (!bitmap || !bitmap->convertToXBGR(SplashBitmap::conversionOpaque)) {
        return false;
    }

    const int w = bitmap->getWidth();
    const int h = bitmap->getHeight();
    const int rowSize = bitmap->getRowSize();

    if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        // Convert byte order from RGBX to XBGR, matching poppler-qt6.
        SplashColorPtr data = bitmap->getDataPtr();
        for (int i = 0; i < h; ++i) {
            for (int j = 0; j < w; ++j) {
                SplashColorPtr pixel = &data[i * rowSize + j];
                std::swap(pixel[0], pixel[3]);
                std::swap(pixel[1], pixel[2]);
            }
        }
    }

    // Copy (rather than take ownership of) the Splash buffer: entangling
    // QImage's allocator expectations with Splash's own buffer lifetime is
    // not worth it here, since this runs once per zoom/rotation change or
    // Dark Reader toggle, not on every repaint.
    const QImage img(bitmap->getDataPtr(), w, h, rowSize, QImage::Format_RGB32);
    *outImage = img.copy();
    *outMask = out.maskImage;

    return !outImage->isNull() && !outMask->isNull();
}
