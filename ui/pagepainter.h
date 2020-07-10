/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_PAGEPAINTER_H_
#define _OKULAR_PAGEPAINTER_H_

#include <QBrush>
#include <QImage>
#include <QPen>

#include "core/annotations.h"
#include "core/area.h" // for NormalizedPoint

class QPainter;
class QRect;
namespace Okular
{
class DocumentObserver;
class Page;
}

/**
 * @short Paints a Okular::Page to an open painter using given flags.
 */
class Q_DECL_EXPORT PagePainter
{
public:
    // list of flags passed to the painting function. by OR-ing those flags
    // you can decide whether or not to permit drawing of a certain feature.
    enum PagePainterFlags { Accessibility = 1, EnhanceLinks = 2, EnhanceImages = 4, Highlights = 8, TextSelection = 16, Annotations = 32 };

    // draw (using painter 'destPainter') the 'page' requested by 'observer' using features
    // in 'flags'. 'limits' is the bounding rect of the paint operation,
    // 'scaledWidth' and 'scaledHeight' the expected size of page contents
    static void paintPageOnPainter(QPainter *destPainter, const Okular::Page *page, Okular::DocumentObserver *observer, int flags, int scaledWidth, int scaledHeight, const QRect pageLimits);

    // draw (using painter 'destPainter') the 'page' requested by 'observer' using features
    // in 'flags'.
    // 'pageLimits' is the bounding rect of the paint operation relative to the
    // top left of the (cropped) page.
    // 'scaledWidth' and 'scaledHeight' the size of the page pixmap (before cropping).
    // 'crop' is the (normalized) cropped rectangle within the page.
    // The painter's (0,0) is assumed to be top left of the painted ('pageLimits') rect.
    static void paintCroppedPageOnPainter(QPainter *destPainter,
                                          const Okular::Page *page,
                                          Okular::DocumentObserver *observer,
                                          int flags,
                                          int scaledWidth,
                                          int scaledHeight,
                                          const QRect pageLimits,
                                          const Okular::NormalizedRect &crop,
                                          Okular::NormalizedPoint *viewPortPoint);

private:
    static void cropPixmapOnImage(QImage &dest, const QPixmap *src, const QRect r);
    static void recolor(QImage *image, const QColor &foreground, const QColor &background);
    static void blackWhite(QImage *image, int contrast, int threshold);
    static void invertLightness(QImage *image);
    /**
     * Inverts luma of @p image using the luma coefficients @p Y_R, @p Y_G, @p Y_B (should sum up to 1),
     * and assuming linear 8bit RGB color space.
     */
    static void invertLuma(QImage *image, float Y_R, float Y_G, float Y_B);
    /**
     * Inverts luma of a pixel given in @p R, @p G, @p B,
     * using the luma coefficients @p Y_R, @p Y_G, @p Y_B (should sum up to 1),
     * and assuming linear 8bit RGB color space.
     */
    static void invertLumaPixel(uchar &R, uchar &G, uchar &B, float Y_R, float Y_G, float Y_B);
    /**
     * Shifts hue of each pixel by 120 degrees, by simply swapping channels.
     */
    static void hueShiftPositive(QImage *image);
    /**
     * Shifts hue of each pixel by 240 degrees, by simply swapping channels.
     */
    static void hueShiftNegative(QImage *image);

    // set the alpha component of the image to a given value
    static void changeImageAlpha(QImage &image, unsigned int alpha);

    // my pretty dear raster function
    typedef QList<Okular::NormalizedPoint> NormalizedPath;
    enum RasterOperation { Normal, Multiply };
    static void drawShapeOnImage(QImage &image, const NormalizedPath &normPath, bool closeShape, const QPen &pen, const QBrush &brush = QBrush(), double penWidthMultiplier = 1.0, RasterOperation op = Normal
                                 // float antiAliasRadius = 1.0
    );
    static void drawEllipseOnImage(QImage &image, const NormalizedPath &rect, const QPen &pen, const QBrush &brush, double penWidthMultiplier, RasterOperation op);

    friend class LineAnnotPainter;
};

class LineAnnotPainter
{
public:
    LineAnnotPainter(const Okular::LineAnnotation *a, QSizeF pageSizeA, double pageScale, const QTransform &toNormalizedImage);
    void draw(QImage &image) const;

private:
    void drawMainLine(QImage &image) const;
    void drawShortenedLine(double mainSegmentLength, double size, QImage &image, const QTransform &toNormalizedPage) const;
    void drawLineEnds(double mainSegmentLength, double size, QImage &image, const QTransform &transform) const;
    void drawLineEndArrow(double xEndPos, double size, double flipX, bool close, const QTransform &toNormalizedPage, QImage &image) const;
    void drawLineEndButt(double xEndPos, double size, const QTransform &toNormalizedPage, QImage &image) const;
    void drawLineEndCircle(double xEndPos, double size, const QTransform &toNormalizedPage, QImage &image) const;
    void drawLineEndSquare(double xEndPos, double size, const QTransform &toNormalizedPage, QImage &image) const;
    void drawLineEndDiamond(double xEndPos, double size, const QTransform &toNormalizedPage, QImage &image) const;
    void drawLineEndSlash(double xEndPos, double size, const QTransform &toNormalizedPage, QImage &image) const;
    void drawLeaderLine(double xEndPos, QImage &image, const QTransform &toNormalizedPage) const;
    template<typename T> QList<Okular::NormalizedPoint> transformPath(const T &path, const QTransform &transform) const
    {
        QList<Okular::NormalizedPoint> transformedPath;
        for (const Okular::NormalizedPoint &item : path) {
            Okular::NormalizedPoint p;
            transform.map(item.x, item.y, &p.x, &p.y);
            transformedPath.append(p);
        }
        return transformedPath;
    }

    static double shortenForArrow(double size, Okular::LineAnnotation::TermStyle endStyle);

private:
    const Okular::LineAnnotation *la;
    QSizeF pageSize;
    double pageScale;
    QTransform toNormalizedImage;
    double aspectRatio;
    const QPen linePen;
    QBrush fillBrush;
};

#endif
