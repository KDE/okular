/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2012 by Guillermo A. Amaral B. <gamaral@kde.org>        *
 *   Copyright (C) 2017    Klarälvdalens Datakonsult AB, a KDAB Group      *
 *                         company, info@kdab.com. Work sponsored by the   *
 *                         LiMux project of the city of Munich             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "annots.h"

#include <poppler-annotation.h>

// qt/kde includes
#include <QLoggingCategory>
#include <QVariant>

#include <core/annotations.h>
#include <core/area.h>

#include "config-okular-poppler.h"
#include "debug_pdf.h"
#include "generator_pdf.h"
#include "popplerembeddedfile.h"

Q_DECLARE_METATYPE(Poppler::Annotation *)

extern Okular::Sound *createSoundFromPopplerSound(const Poppler::SoundObject *popplerSound);
extern Okular::Movie *createMovieFromPopplerMovie(const Poppler::MovieObject *popplerMovie);
extern Okular::Movie *createMovieFromPopplerScreen(const Poppler::LinkRendition *popplerScreen);
extern QPair<Okular::Movie *, Okular::EmbeddedFile *> createMovieFromPopplerRichMedia(const Poppler::RichMediaAnnotation *popplerRichMedia);

static void disposeAnnotation(const Okular::Annotation *ann)
{
    Poppler::Annotation *popplerAnn = qvariant_cast<Poppler::Annotation *>(ann->nativeId());
    delete popplerAnn;
}

static QPointF normPointToPointF(const Okular::NormalizedPoint &pt)
{
    return QPointF(pt.x, pt.y);
}

static QRectF normRectToRectF(const Okular::NormalizedRect &rect)
{
    return QRectF(QPointF(rect.left, rect.top), QPointF(rect.right, rect.bottom));
}

// Poppler and Okular share the same flag values, but we don't want to export internal flags
static int maskExportedFlags(int flags)
{
    return flags &
        (Okular::Annotation::Hidden | Okular::Annotation::FixedSize | Okular::Annotation::FixedRotation | Okular::Annotation::DenyPrint | Okular::Annotation::DenyWrite | Okular::Annotation::DenyDelete |
         Okular::Annotation::ToggleHidingOnMouse);
}

// BEGIN PopplerAnnotationProxy implementation
PopplerAnnotationProxy::PopplerAnnotationProxy(Poppler::Document *doc, QMutex *userMutex, QHash<Okular::Annotation *, Poppler::Annotation *> *annotsOnOpenHash)
    : ppl_doc(doc)
    , mutex(userMutex)
    , annotationsOnOpenHash(annotsOnOpenHash)
{
}

PopplerAnnotationProxy::~PopplerAnnotationProxy()
{
}

bool PopplerAnnotationProxy::supports(Capability cap) const
{
    switch (cap) {
    case Addition:
    case Modification:
    case Removal:
        return true;
    default:
        return false;
    }
}

void PopplerAnnotationProxy::notifyAddition(Okular::Annotation *okl_ann, int page)
{
    // Export annotation to DOM
    QDomDocument doc;
    QDomElement dom_ann = doc.createElement(QStringLiteral("root"));
    Okular::AnnotationUtils::storeAnnotation(okl_ann, dom_ann, doc);

    QMutexLocker ml(mutex);

    // Create poppler annotation
    Poppler::Annotation *ppl_ann = Poppler::AnnotationUtils::createAnnotation(dom_ann);

    // Poppler doesn't render StampAnnotations yet
    if (ppl_ann->subType() != Poppler::Annotation::AStamp)
        okl_ann->setFlags(okl_ann->flags() | Okular::Annotation::ExternallyDrawn);

    // Poppler stores highlight points in swapped order
    if (ppl_ann->subType() == Poppler::Annotation::AHighlight) {
        Poppler::HighlightAnnotation *hlann = static_cast<Poppler::HighlightAnnotation *>(ppl_ann);
        QList<Poppler::HighlightAnnotation::Quad> quads = hlann->highlightQuads();
        QMutableListIterator<Poppler::HighlightAnnotation::Quad> it(quads);
        while (it.hasNext()) {
            Poppler::HighlightAnnotation::Quad &q = it.next();
            QPointF t;
            t = q.points[3];
            q.points[3] = q.points[0];
            q.points[0] = t;
            t = q.points[2];
            q.points[2] = q.points[1];
            q.points[1] = t;
        }
        hlann->setHighlightQuads(quads);
    }

    // Bind poppler object to page
    Poppler::Page *ppl_page = ppl_doc->page(page);
    ppl_page->addAnnotation(ppl_ann);
    delete ppl_page;

    // Set pointer to poppler annotation as native Id
    okl_ann->setNativeId(QVariant::fromValue(ppl_ann));
    okl_ann->setDisposeDataFunction(disposeAnnotation);

    qCDebug(OkularPdfDebug) << okl_ann->uniqueName();
}

void PopplerAnnotationProxy::notifyModification(const Okular::Annotation *okl_ann, int page, bool appearanceChanged)
{
    Q_UNUSED(page);
    Q_UNUSED(appearanceChanged);

    Poppler::Annotation *ppl_ann = qvariant_cast<Poppler::Annotation *>(okl_ann->nativeId());

    if (!ppl_ann) // Ignore non-native annotations
        return;

    QMutexLocker ml(mutex);

    if (okl_ann->flags() & (Okular::Annotation::BeingMoved | Okular::Annotation::BeingResized)) {
        // Okular ui already renders the annotation on its own
        ppl_ann->setFlags(Poppler::Annotation::Hidden);
        return;
    }

    // Set basic properties
    // Note: flags and boundary must be set first in order to correctly handle
    // FixedRotation annotations.
    ppl_ann->setFlags(maskExportedFlags(okl_ann->flags()));
    ppl_ann->setBoundary(normRectToRectF(okl_ann->boundingRectangle()));

    ppl_ann->setAuthor(okl_ann->author());
    ppl_ann->setContents(okl_ann->contents());

    // Set style
    Poppler::Annotation::Style s;
    s.setColor(okl_ann->style().color());
    s.setWidth(okl_ann->style().width());
    s.setOpacity(okl_ann->style().opacity());
    ppl_ann->setStyle(s);

    // Set type-specific properties (if any)
    switch (ppl_ann->subType()) {
    case Poppler::Annotation::AText: {
        const Okular::TextAnnotation *okl_txtann = static_cast<const Okular::TextAnnotation *>(okl_ann);
        Poppler::TextAnnotation *ppl_txtann = static_cast<Poppler::TextAnnotation *>(ppl_ann);
        ppl_txtann->setTextIcon(okl_txtann->textIcon());
        ppl_txtann->setTextFont(okl_txtann->textFont());
#ifdef HAVE_POPPLER_0_69
        ppl_txtann->setTextColor(okl_txtann->textColor());
#endif // HAVE_POPPLER_0_69
        ppl_txtann->setInplaceAlign(okl_txtann->inplaceAlignment());
        ppl_txtann->setCalloutPoints(QVector<QPointF>());
        ppl_txtann->setInplaceIntent((Poppler::TextAnnotation::InplaceIntent)okl_txtann->inplaceIntent());
        break;
    }
    case Poppler::Annotation::ALine: {
        const Okular::LineAnnotation *okl_lineann = static_cast<const Okular::LineAnnotation *>(okl_ann);
        Poppler::LineAnnotation *ppl_lineann = static_cast<Poppler::LineAnnotation *>(ppl_ann);
        QLinkedList<QPointF> points;
        const QLinkedList<Okular::NormalizedPoint> annotPoints = okl_lineann->linePoints();
        for (const Okular::NormalizedPoint &p : annotPoints) {
            points.append(normPointToPointF(p));
        }
        ppl_lineann->setLinePoints(points);
        ppl_lineann->setLineStartStyle((Poppler::LineAnnotation::TermStyle)okl_lineann->lineStartStyle());
        ppl_lineann->setLineEndStyle((Poppler::LineAnnotation::TermStyle)okl_lineann->lineEndStyle());
        ppl_lineann->setLineClosed(okl_lineann->lineClosed());
        ppl_lineann->setLineInnerColor(okl_lineann->lineInnerColor());
        ppl_lineann->setLineLeadingForwardPoint(okl_lineann->lineLeadingForwardPoint());
        ppl_lineann->setLineLeadingBackPoint(okl_lineann->lineLeadingBackwardPoint());
        ppl_lineann->setLineShowCaption(okl_lineann->showCaption());
        ppl_lineann->setLineIntent((Poppler::LineAnnotation::LineIntent)okl_lineann->lineIntent());
        break;
    }
    case Poppler::Annotation::AGeom: {
        const Okular::GeomAnnotation *okl_geomann = static_cast<const Okular::GeomAnnotation *>(okl_ann);
        Poppler::GeomAnnotation *ppl_geomann = static_cast<Poppler::GeomAnnotation *>(ppl_ann);
        ppl_geomann->setGeomType((Poppler::GeomAnnotation::GeomType)okl_geomann->geometricalType());
        ppl_geomann->setGeomInnerColor(okl_geomann->geometricalInnerColor());
        break;
    }
    case Poppler::Annotation::AHighlight: {
        const Okular::HighlightAnnotation *okl_hlann = static_cast<const Okular::HighlightAnnotation *>(okl_ann);
        Poppler::HighlightAnnotation *ppl_hlann = static_cast<Poppler::HighlightAnnotation *>(ppl_ann);
        ppl_hlann->setHighlightType((Poppler::HighlightAnnotation::HighlightType)okl_hlann->highlightType());
        break;
    }
    case Poppler::Annotation::AStamp: {
        const Okular::StampAnnotation *okl_stampann = static_cast<const Okular::StampAnnotation *>(okl_ann);
        Poppler::StampAnnotation *ppl_stampann = static_cast<Poppler::StampAnnotation *>(ppl_ann);
        ppl_stampann->setStampIconName(okl_stampann->stampIconName());
        break;
    }
    case Poppler::Annotation::AInk: {
        const Okular::InkAnnotation *okl_inkann = static_cast<const Okular::InkAnnotation *>(okl_ann);
        Poppler::InkAnnotation *ppl_inkann = static_cast<Poppler::InkAnnotation *>(ppl_ann);
        QList<QLinkedList<QPointF>> paths;
        const QList<QLinkedList<Okular::NormalizedPoint>> inkPathsList = okl_inkann->inkPaths();
        for (const QLinkedList<Okular::NormalizedPoint> &path : inkPathsList) {
            QLinkedList<QPointF> points;
            for (const Okular::NormalizedPoint &p : path) {
                points.append(normPointToPointF(p));
            }
            paths.append(points);
        }
        ppl_inkann->setInkPaths(paths);
        break;
    }
    default:
        qCDebug(OkularPdfDebug) << "Type-specific property modification is not implemented for this annotation type";
        break;
    }

    qCDebug(OkularPdfDebug) << okl_ann->uniqueName();
}

void PopplerAnnotationProxy::notifyRemoval(Okular::Annotation *okl_ann, int page)
{
    Poppler::Annotation *ppl_ann = qvariant_cast<Poppler::Annotation *>(okl_ann->nativeId());

    if (!ppl_ann) // Ignore non-native annotations
        return;

    QMutexLocker ml(mutex);

    Poppler::Page *ppl_page = ppl_doc->page(page);
    annotationsOnOpenHash->remove(okl_ann);
    ppl_page->removeAnnotation(ppl_ann); // Also destroys ppl_ann
    delete ppl_page;

    okl_ann->setNativeId(QVariant::fromValue(0)); // So that we don't double-free in disposeAnnotation

    qCDebug(OkularPdfDebug) << okl_ann->uniqueName();
}
// END PopplerAnnotationProxy implementation

static Okular::Annotation::LineStyle popplerToOkular(Poppler::Annotation::LineStyle s)
{
    switch (s) {
    case Poppler::Annotation::Solid:
        return Okular::Annotation::Solid;
    case Poppler::Annotation::Dashed:
        return Okular::Annotation::Dashed;
    case Poppler::Annotation::Beveled:
        return Okular::Annotation::Beveled;
    case Poppler::Annotation::Inset:
        return Okular::Annotation::Inset;
    case Poppler::Annotation::Underline:
        return Okular::Annotation::Underline;
    default:
        qWarning() << Q_FUNC_INFO << "unknown value" << s;
    }

    return Okular::Annotation::Solid;
}

static Okular::Annotation::LineEffect popplerToOkular(Poppler::Annotation::LineEffect e)
{
    switch (e) {
    case Poppler::Annotation::NoEffect:
        return Okular::Annotation::NoEffect;
    case Poppler::Annotation::Cloudy:
        return Okular::Annotation::Cloudy;
    default:
        qWarning() << Q_FUNC_INFO << "unknown value" << e;
    }

    return Okular::Annotation::NoEffect;
}

static Okular::Annotation::RevisionScope popplerToOkular(Poppler::Annotation::RevScope s)
{
    switch (s) {
    case Poppler::Annotation::Root:
        Q_UNREACHABLE();
    case Poppler::Annotation::Reply:
        return Okular::Annotation::Reply;
    case Poppler::Annotation::Group:
        return Okular::Annotation::Group;
    case Poppler::Annotation::Delete:
        return Okular::Annotation::Delete;
    default:
        qWarning() << Q_FUNC_INFO << "unknown value" << s;
    }

    return Okular::Annotation::Reply;
}

static Okular::Annotation::RevisionType popplerToOkular(Poppler::Annotation::RevType t)
{
    switch (t) {
    case Poppler::Annotation::None:
        return Okular::Annotation::None;
    case Poppler::Annotation::Marked:
        return Okular::Annotation::Marked;
    case Poppler::Annotation::Unmarked:
        return Okular::Annotation::Unmarked;
    case Poppler::Annotation::Accepted:
        return Okular::Annotation::Accepted;
    case Poppler::Annotation::Rejected:
        return Okular::Annotation::Rejected;
    case Poppler::Annotation::Cancelled:
        return Okular::Annotation::Cancelled;
    case Poppler::Annotation::Completed:
        return Okular::Annotation::Completed;
    default:
        qWarning() << Q_FUNC_INFO << "unknown value" << t;
    }

    return Okular::Annotation::None;
}

static Okular::TextAnnotation::TextType popplerToOkular(Poppler::TextAnnotation::TextType ptt)
{
    switch (ptt) {
    case Poppler::TextAnnotation::Linked:
        return Okular::TextAnnotation::Linked;
    case Poppler::TextAnnotation::InPlace:
        return Okular::TextAnnotation::Linked;
    default:
        qWarning() << Q_FUNC_INFO << "unknown value" << ptt;
    }

    return Okular::TextAnnotation::Linked;
}

static Okular::TextAnnotation::InplaceIntent popplerToOkular(Poppler::TextAnnotation::InplaceIntent pii)
{
    switch (pii) {
    case Poppler::TextAnnotation::Unknown:
        return Okular::TextAnnotation::Unknown;
    case Poppler::TextAnnotation::Callout:
        return Okular::TextAnnotation::Callout;
    case Poppler::TextAnnotation::TypeWriter:
        return Okular::TextAnnotation::TypeWriter;
    default:
        qWarning() << Q_FUNC_INFO << "unknown value" << pii;
    }

    return Okular::TextAnnotation::Unknown;
}

static Okular::LineAnnotation::TermStyle popplerToOkular(Poppler::LineAnnotation::TermStyle pts)
{
    switch (pts) {
    case Poppler::LineAnnotation::Square:
        return Okular::LineAnnotation::Square;
    case Poppler::LineAnnotation::Circle:
        return Okular::LineAnnotation::Circle;
    case Poppler::LineAnnotation::Diamond:
        return Okular::LineAnnotation::Diamond;
    case Poppler::LineAnnotation::OpenArrow:
        return Okular::LineAnnotation::OpenArrow;
    case Poppler::LineAnnotation::ClosedArrow:
        return Okular::LineAnnotation::ClosedArrow;
    case Poppler::LineAnnotation::None:
        return Okular::LineAnnotation::None;
    case Poppler::LineAnnotation::Butt:
        return Okular::LineAnnotation::Butt;
    case Poppler::LineAnnotation::ROpenArrow:
        return Okular::LineAnnotation::ROpenArrow;
    case Poppler::LineAnnotation::RClosedArrow:
        return Okular::LineAnnotation::RClosedArrow;
    case Poppler::LineAnnotation::Slash:
        return Okular::LineAnnotation::Slash;
    default:
        qWarning() << Q_FUNC_INFO << "unknown value" << pts;
    }

    return Okular::LineAnnotation::None;
}

static Okular::LineAnnotation::LineIntent popplerToOkular(Poppler::LineAnnotation::LineIntent pli)
{
    switch (pli) {
    case Poppler::LineAnnotation::Unknown:
        return Okular::LineAnnotation::Unknown;
    case Poppler::LineAnnotation::Arrow:
        return Okular::LineAnnotation::Arrow;
    case Poppler::LineAnnotation::Dimension:
        return Okular::LineAnnotation::Dimension;
    case Poppler::LineAnnotation::PolygonCloud:
        return Okular::LineAnnotation::PolygonCloud;
    default:
        qWarning() << Q_FUNC_INFO << "unknown value" << pli;
    }

    return Okular::LineAnnotation::Unknown;
}

static Okular::Annotation *createAnnotationFromPopplerAnnotation(Poppler::TextAnnotation *popplerAnnotation)
{
    Okular::TextAnnotation *oTextAnn = new Okular::TextAnnotation();

    oTextAnn->setTextType(popplerToOkular(popplerAnnotation->textType()));
    oTextAnn->setTextIcon(popplerAnnotation->textIcon());
    oTextAnn->setTextFont(popplerAnnotation->textFont());
#ifdef HAVE_POPPLER_0_69
    oTextAnn->setTextColor(popplerAnnotation->textColor());
#endif
    // this works because we use the same 0:left, 1:center, 2:right meaning both in poppler and okular
    oTextAnn->setInplaceAlignment(popplerAnnotation->inplaceAlign());
    oTextAnn->setInplaceIntent(popplerToOkular(popplerAnnotation->inplaceIntent()));
    for (int i = 0; i < 3; ++i) {
        const QPointF p = popplerAnnotation->calloutPoint(i);
        oTextAnn->setInplaceCallout({p.x(), p.y()}, i);
    }

    return oTextAnn;
}

static Okular::Annotation *createAnnotationFromPopplerAnnotation(const Poppler::LineAnnotation *popplerAnnotation)
{
    Okular::LineAnnotation *oLineAnn = new Okular::LineAnnotation();

    oLineAnn->setLineStartStyle(popplerToOkular(popplerAnnotation->lineStartStyle()));
    oLineAnn->setLineEndStyle(popplerToOkular(popplerAnnotation->lineEndStyle()));
    oLineAnn->setLineClosed(popplerAnnotation->isLineClosed());
    oLineAnn->setLineInnerColor(popplerAnnotation->lineInnerColor());
    oLineAnn->setLineLeadingForwardPoint(popplerAnnotation->lineLeadingForwardPoint());
    oLineAnn->setLineLeadingBackwardPoint(popplerAnnotation->lineLeadingBackPoint());
    oLineAnn->setShowCaption(popplerAnnotation->lineShowCaption());
    oLineAnn->setLineIntent(popplerToOkular(popplerAnnotation->lineIntent()));

    QLinkedList<Okular::NormalizedPoint> points;
    const QLinkedList<QPointF> popplerPoints = popplerAnnotation->linePoints();
    for (const QPointF &p : popplerPoints) {
        points << Okular::NormalizedPoint(p.x(), p.y());
    }
    oLineAnn->setLinePoints(points);

    return oLineAnn;
}

Okular::Annotation *createAnnotationFromPopplerAnnotation(Poppler::Annotation *popplerAnnotation, const Poppler::Page &popplerPage, bool *doDelete)
{
    Okular::Annotation *okularAnnotation = nullptr;
    *doDelete = true;
    bool tieToOkularAnn = false;
    bool externallyDrawn = false;
    switch (popplerAnnotation->subType()) {
    case Poppler::Annotation::AFileAttachment: {
        Poppler::FileAttachmentAnnotation *attachann = static_cast<Poppler::FileAttachmentAnnotation *>(popplerAnnotation);
        Okular::FileAttachmentAnnotation *f = new Okular::FileAttachmentAnnotation();
        okularAnnotation = f;
        tieToOkularAnn = true;
        *doDelete = false;

        f->setFileIconName(attachann->fileIconName());
        f->setEmbeddedFile(new PDFEmbeddedFile(attachann->embeddedFile()));

        break;
    }
    case Poppler::Annotation::ASound: {
        Poppler::SoundAnnotation *soundann = static_cast<Poppler::SoundAnnotation *>(popplerAnnotation);
        Okular::SoundAnnotation *s = new Okular::SoundAnnotation();
        okularAnnotation = s;

        s->setSoundIconName(soundann->soundIconName());
        s->setSound(createSoundFromPopplerSound(soundann->sound()));

        break;
    }
    case Poppler::Annotation::AMovie: {
        Poppler::MovieAnnotation *movieann = static_cast<Poppler::MovieAnnotation *>(popplerAnnotation);
        Okular::MovieAnnotation *m = new Okular::MovieAnnotation();
        okularAnnotation = m;
        tieToOkularAnn = true;
        *doDelete = false;

        m->setMovie(createMovieFromPopplerMovie(movieann->movie()));

        break;
    }
    case Poppler::Annotation::AWidget: {
        okularAnnotation = new Okular::WidgetAnnotation();
        break;
    }
    case Poppler::Annotation::AScreen: {
        Okular::ScreenAnnotation *m = new Okular::ScreenAnnotation();
        okularAnnotation = m;
        tieToOkularAnn = true;
        *doDelete = false;
        break;
    }
    case Poppler::Annotation::ARichMedia: {
        Poppler::RichMediaAnnotation *richmediaann = static_cast<Poppler::RichMediaAnnotation *>(popplerAnnotation);
        const QPair<Okular::Movie *, Okular::EmbeddedFile *> result = createMovieFromPopplerRichMedia(richmediaann);

        if (result.first) {
            Okular::RichMediaAnnotation *r = new Okular::RichMediaAnnotation();
            tieToOkularAnn = true;
            *doDelete = false;
            okularAnnotation = r;

            r->setMovie(result.first);
            r->setEmbeddedFile(result.second);
        }

        break;
    }
    case Poppler::Annotation::AText: {
        externallyDrawn = true;
        tieToOkularAnn = true;
        *doDelete = false;
        okularAnnotation = createAnnotationFromPopplerAnnotation(static_cast<Poppler::TextAnnotation *>(popplerAnnotation));
        break;
    }
    case Poppler::Annotation::ALine: {
        externallyDrawn = true;
        tieToOkularAnn = true;
        *doDelete = false;
        okularAnnotation = createAnnotationFromPopplerAnnotation(static_cast<Poppler::LineAnnotation *>(popplerAnnotation));
        break;
    }
    case Poppler::Annotation::AGeom:
    case Poppler::Annotation::AHighlight:
    case Poppler::Annotation::AInk:
    case Poppler::Annotation::ACaret:
        externallyDrawn = true;
        /* fallthrough */
    case Poppler::Annotation::AStamp:
        tieToOkularAnn = true;
        *doDelete = false;
        /* fallthrough */
    default: {
        // this is uber ugly but i don't know a better way to do it without introducing a poppler::annotation dependency on core
        QDomDocument doc;
        QDomElement root = doc.createElement(QStringLiteral("root"));
        doc.appendChild(root);
        Poppler::AnnotationUtils::storeAnnotation(popplerAnnotation, root, doc);
        okularAnnotation = Okular::AnnotationUtils::createAnnotation(root);
        break;
    }
    }
    if (okularAnnotation) {
        // the Contents field might have lines separated by \r
        QString contents = popplerAnnotation->contents();
        contents.replace(QLatin1Char('\r'), QLatin1Char('\n'));

        okularAnnotation->setAuthor(popplerAnnotation->author());
        okularAnnotation->setContents(contents);
        okularAnnotation->setUniqueName(popplerAnnotation->uniqueName());
        okularAnnotation->setModificationDate(popplerAnnotation->modificationDate());
        okularAnnotation->setCreationDate(popplerAnnotation->creationDate());
        okularAnnotation->setFlags(popplerAnnotation->flags() | Okular::Annotation::External);
        okularAnnotation->setBoundingRectangle(Okular::NormalizedRect::fromQRectF(popplerAnnotation->boundary()));

        if (externallyDrawn)
            okularAnnotation->setFlags(okularAnnotation->flags() | Okular::Annotation::ExternallyDrawn);

        // Poppler stores highlight points in swapped order
        if (okularAnnotation->subType() == Okular::Annotation::AHighlight) {
            Okular::HighlightAnnotation *hlann = static_cast<Okular::HighlightAnnotation *>(okularAnnotation);
            for (Okular::HighlightAnnotation::Quad &quad : hlann->highlightQuads()) {
                const Okular::NormalizedPoint p3 = quad.point(3);
                quad.setPoint(quad.point(0), 3);
                quad.setPoint(p3, 0);
                const Okular::NormalizedPoint p2 = quad.point(2);
                quad.setPoint(quad.point(1), 2);
                quad.setPoint(p2, 1);
            }
        }

        if (okularAnnotation->subType() == Okular::Annotation::AText) {
            Okular::TextAnnotation *txtann = static_cast<Okular::TextAnnotation *>(okularAnnotation);

            if (txtann->textType() == Okular::TextAnnotation::Linked) {
                Poppler::TextAnnotation *ppl_txtann = static_cast<Poppler::TextAnnotation *>(popplerAnnotation);

                // Poppler and Okular assume a different default icon name in XML
                // We re-read it via getter, which always tells the right one
                txtann->setTextIcon(ppl_txtann->textIcon());
            }
        }

        // Convert the poppler annotation style to Okular annotation style
        Okular::Annotation::Style &okularStyle = okularAnnotation->style();
        const Poppler::Annotation::Style popplerStyle = popplerAnnotation->style();
        okularStyle.setColor(popplerStyle.color());
        okularStyle.setOpacity(popplerStyle.opacity());
        okularStyle.setWidth(popplerStyle.width());
        okularStyle.setLineStyle(popplerToOkular(popplerStyle.lineStyle()));
        okularStyle.setXCorners(popplerStyle.xCorners());
        okularStyle.setYCorners(popplerStyle.yCorners());
        const QVector<double> &dashArray = popplerStyle.dashArray();
        if (dashArray.size() > 0)
            okularStyle.setMarks(dashArray[0]);
        if (dashArray.size() > 1)
            okularStyle.setSpaces(dashArray[1]);
        okularStyle.setLineEffect(popplerToOkular(popplerStyle.lineEffect()));
        okularStyle.setEffectIntensity(popplerStyle.effectIntensity());

        // Convert the poppler annotation popup to Okular annotation window
        Okular::Annotation::Window &okularWindow = okularAnnotation->window();
        const Poppler::Annotation::Popup popplerPopup = popplerAnnotation->popup();
        // This assumes that both "flags" int mean the same, but since we don't use the flags in okular anywhere it's not really that important
        okularWindow.setFlags(popplerPopup.flags());
        const QRectF popplerGeometry = popplerPopup.geometry();
        const QSize popplerPageSize = popplerPage.pageSize();
        okularWindow.setTopLeft(Okular::NormalizedPoint(popplerGeometry.top(), popplerGeometry.left(), popplerPageSize.width(), popplerPageSize.height()));
        okularWindow.setWidth(popplerGeometry.width());
        okularWindow.setHeight(popplerGeometry.height());
        okularWindow.setTitle(popplerPopup.title());
        okularWindow.setSummary(popplerPopup.summary());

        // Convert the poppler revisions to Okular revisions
        QLinkedList<Okular::Annotation::Revision> &okularRevisions = okularAnnotation->revisions();
        okularRevisions.clear(); // We can remove this once we stop calling Okular::AnnotationUtils::createAnnotation
        const QList<Poppler::Annotation *> popplerRevisions = popplerAnnotation->revisions();
        for (Poppler::Annotation *popplerRevision : popplerRevisions) {
            bool deletePopplerRevision;
            Okular::Annotation::Revision okularRevision;
            okularRevision.setAnnotation(createAnnotationFromPopplerAnnotation(popplerRevision, popplerPage, &deletePopplerRevision));
            okularRevision.setScope(popplerToOkular(popplerRevision->revisionScope()));
            okularRevision.setType(popplerToOkular(popplerRevision->revisionType()));
            okularRevisions << okularRevision;

            if (deletePopplerRevision) {
                delete popplerRevision;
            }
        }

        if (tieToOkularAnn) {
            okularAnnotation->setNativeId(QVariant::fromValue(popplerAnnotation));
            okularAnnotation->setDisposeDataFunction(disposeAnnotation);
        }
    }
    return okularAnnotation;
}
