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

#include "debug_pdf.h"
#include "generator_pdf.h"
#include "popplerembeddedfile.h"
#include "config-okular-poppler.h"

Q_DECLARE_METATYPE( Poppler::Annotation* )

extern Okular::Sound* createSoundFromPopplerSound( const Poppler::SoundObject *popplerSound );
extern Okular::Movie* createMovieFromPopplerMovie( const Poppler::MovieObject *popplerMovie );
extern Okular::Movie* createMovieFromPopplerScreen( const Poppler::LinkRendition *popplerScreen );
#ifdef HAVE_POPPLER_0_36
extern QPair<Okular::Movie*, Okular::EmbeddedFile*> createMovieFromPopplerRichMedia( const Poppler::RichMediaAnnotation *popplerRichMedia );
#endif


static void disposeAnnotation( const Okular::Annotation *ann )
{
    Poppler::Annotation *popplerAnn = qvariant_cast< Poppler::Annotation * >( ann->nativeId() );
    delete popplerAnn;
}

static QPointF normPointToPointF( const Okular::NormalizedPoint& pt )
{
    return QPointF(pt.x, pt.y);
}

static QRectF normRectToRectF( const Okular::NormalizedRect& rect )
{
    return QRectF( QPointF(rect.left, rect.top), QPointF(rect.right, rect.bottom) );
}

// Poppler and Okular share the same flag values, but we don't want to export internal flags
static int maskExportedFlags(int flags)
{
    return flags & ( Okular::Annotation::Hidden |
                     Okular::Annotation::FixedSize |
                     Okular::Annotation::FixedRotation |
                     Okular::Annotation::DenyPrint |
                     Okular::Annotation::DenyWrite |
                     Okular::Annotation::DenyDelete |
                     Okular::Annotation::ToggleHidingOnMouse );
}

//BEGIN PopplerAnnotationProxy implementation
PopplerAnnotationProxy::PopplerAnnotationProxy( Poppler::Document *doc, QMutex *userMutex, QHash<Okular::Annotation*, Poppler::Annotation*> *annotsOnOpenHash )
    : ppl_doc ( doc ), mutex ( userMutex ), annotationsOnOpenHash( annotsOnOpenHash )
{
}

PopplerAnnotationProxy::~PopplerAnnotationProxy()
{
}

bool PopplerAnnotationProxy::supports( Capability cap ) const
{
    switch ( cap )
    {
        case Addition:
        case Modification:
        case Removal:
            return true;
        default:
            return false;
    }
}

void PopplerAnnotationProxy::notifyAddition( Okular::Annotation *okl_ann, int page )
{
    // Export annotation to DOM
    QDomDocument doc;
    QDomElement dom_ann = doc.createElement( QStringLiteral("root") );
    Okular::AnnotationUtils::storeAnnotation( okl_ann, dom_ann, doc );

    QMutexLocker ml(mutex);

    // Create poppler annotation
    Poppler::Annotation *ppl_ann = Poppler::AnnotationUtils::createAnnotation( dom_ann );

    // Poppler doesn't render StampAnnotations yet
    if ( ppl_ann->subType() != Poppler::Annotation::AStamp )
        okl_ann->setFlags( okl_ann->flags() | Okular::Annotation::ExternallyDrawn );

    // Poppler stores highlight points in swapped order
    if ( ppl_ann->subType() == Poppler::Annotation::AHighlight )
    {
        Poppler::HighlightAnnotation * hlann = static_cast<Poppler::HighlightAnnotation*>( ppl_ann );
        QList<Poppler::HighlightAnnotation::Quad> quads = hlann->highlightQuads();
        QMutableListIterator<Poppler::HighlightAnnotation::Quad> it( quads );
        while ( it.hasNext() )
        {
            Poppler::HighlightAnnotation::Quad &q = it.next();
            QPointF t;
            t = q.points[3];
            q.points[3] = q.points[0];
            q.points[0] = t;
            t = q.points[2];
            q.points[2] = q.points[1];
            q.points[1] = t;
        }
        hlann->setHighlightQuads( quads );
    }

    // Bind poppler object to page
    Poppler::Page *ppl_page = ppl_doc->page( page );
    ppl_page->addAnnotation( ppl_ann );
    delete ppl_page;

    // Set pointer to poppler annotation as native Id
    okl_ann->setNativeId( QVariant::fromValue( ppl_ann ) );
    okl_ann->setDisposeDataFunction( disposeAnnotation );

    qCDebug(OkularPdfDebug) << okl_ann->uniqueName();
}

void PopplerAnnotationProxy::notifyModification( const Okular::Annotation *okl_ann, int page, bool appearanceChanged )
{
    Q_UNUSED( page );
    Q_UNUSED( appearanceChanged );

    Poppler::Annotation *ppl_ann = qvariant_cast<Poppler::Annotation*>( okl_ann->nativeId() );

    if ( !ppl_ann ) // Ignore non-native annotations
        return;

    QMutexLocker ml(mutex);

    if ( okl_ann->flags() & (Okular::Annotation::BeingMoved | Okular::Annotation::BeingResized) )
    {
        // Okular ui already renders the annotation on its own
        ppl_ann->setFlags( Poppler::Annotation::Hidden );
        return;
    }

    // Set basic properties
    // Note: flags and boundary must be set first in order to correctly handle
    // FixedRotation annotations.
    ppl_ann->setFlags(maskExportedFlags( okl_ann->flags() ));
    ppl_ann->setBoundary(normRectToRectF( okl_ann->boundingRectangle() ));

    ppl_ann->setAuthor( okl_ann->author() );
    ppl_ann->setContents( okl_ann->contents() );

    // Set style
    Poppler::Annotation::Style s;
    s.setColor( okl_ann->style().color() );
    s.setWidth( okl_ann->style().width() );
    s.setOpacity( okl_ann->style().opacity() );
    ppl_ann->setStyle( s );

    // Set type-specific properties (if any)
    switch ( ppl_ann->subType() )
    {
        case Poppler::Annotation::AText:
        {
            const Okular::TextAnnotation * okl_txtann = static_cast<const Okular::TextAnnotation*>(okl_ann);
            Poppler::TextAnnotation * ppl_txtann = static_cast<Poppler::TextAnnotation*>(ppl_ann);
            ppl_txtann->setTextIcon( okl_txtann->textIcon() );
            ppl_txtann->setTextFont( okl_txtann->textFont() );
            #ifdef HAVE_POPPLER_0_69
            ppl_txtann->setTextColor( okl_txtann->textColor() );
            #endif //HAVE_POPPLER_0_69
            ppl_txtann->setInplaceAlign( okl_txtann->inplaceAlignment() );
            ppl_txtann->setCalloutPoints( QVector<QPointF>() );
            ppl_txtann->setInplaceIntent( (Poppler::TextAnnotation::InplaceIntent)okl_txtann->inplaceIntent() );
            break;
        }
        case Poppler::Annotation::ALine:
        {
            const Okular::LineAnnotation * okl_lineann = static_cast<const Okular::LineAnnotation*>(okl_ann);
            Poppler::LineAnnotation * ppl_lineann = static_cast<Poppler::LineAnnotation*>(ppl_ann);
            QLinkedList<QPointF> points;
            foreach ( const Okular::NormalizedPoint &p, okl_lineann->linePoints() )
                points.append(normPointToPointF( p ));
            ppl_lineann->setLinePoints( points );
            ppl_lineann->setLineStartStyle( (Poppler::LineAnnotation::TermStyle)okl_lineann->lineStartStyle() );
            ppl_lineann->setLineEndStyle( (Poppler::LineAnnotation::TermStyle)okl_lineann->lineEndStyle() );
            ppl_lineann->setLineClosed( okl_lineann->lineClosed() );
            ppl_lineann->setLineInnerColor( okl_lineann->lineInnerColor() );
            ppl_lineann->setLineLeadingForwardPoint( okl_lineann->lineLeadingForwardPoint() );
            ppl_lineann->setLineLeadingBackPoint( okl_lineann->lineLeadingBackwardPoint() );
            ppl_lineann->setLineShowCaption( okl_lineann->showCaption() );
            ppl_lineann->setLineIntent( (Poppler::LineAnnotation::LineIntent)okl_lineann->lineIntent() );
            break;
        }
        case Poppler::Annotation::AGeom:
        {
            const Okular::GeomAnnotation * okl_geomann = static_cast<const Okular::GeomAnnotation*>(okl_ann);
            Poppler::GeomAnnotation * ppl_geomann = static_cast<Poppler::GeomAnnotation*>(ppl_ann);
            ppl_geomann->setGeomType( (Poppler::GeomAnnotation::GeomType)okl_geomann->geometricalType() );
            ppl_geomann->setGeomInnerColor( okl_geomann->geometricalInnerColor() );
            break;
        }
        case Poppler::Annotation::AHighlight:
        {
            const Okular::HighlightAnnotation * okl_hlann = static_cast<const Okular::HighlightAnnotation*>(okl_ann);
            Poppler::HighlightAnnotation * ppl_hlann = static_cast<Poppler::HighlightAnnotation*>(ppl_ann);
            ppl_hlann->setHighlightType( (Poppler::HighlightAnnotation::HighlightType)okl_hlann->highlightType() );
            break;
        }
        case Poppler::Annotation::AStamp:
        {
            const Okular::StampAnnotation * okl_stampann = static_cast<const Okular::StampAnnotation*>(okl_ann);
            Poppler::StampAnnotation * ppl_stampann = static_cast<Poppler::StampAnnotation*>(ppl_ann);
            ppl_stampann->setStampIconName( okl_stampann->stampIconName() );
            break;
        }
        case Poppler::Annotation::AInk:
        {
            const Okular::InkAnnotation * okl_inkann = static_cast<const Okular::InkAnnotation*>(okl_ann);
            Poppler::InkAnnotation * ppl_inkann = static_cast<Poppler::InkAnnotation*>(ppl_ann);
            QList< QLinkedList<QPointF> > paths;
            foreach ( const QLinkedList<Okular::NormalizedPoint> &path, okl_inkann->inkPaths() )
            {
                QLinkedList<QPointF> points;
                foreach ( const Okular::NormalizedPoint &p, path )
                    points.append(normPointToPointF( p ));
                paths.append( points );
            }
            ppl_inkann->setInkPaths( paths );
            break;
        }
        default:
            qCDebug(OkularPdfDebug) << "Type-specific property modification is not implemented for this annotation type";
            break;
    }

    qCDebug(OkularPdfDebug) << okl_ann->uniqueName();
}

void PopplerAnnotationProxy::notifyRemoval( Okular::Annotation *okl_ann, int page )
{
    Poppler::Annotation *ppl_ann = qvariant_cast<Poppler::Annotation*>( okl_ann->nativeId() );

    if ( !ppl_ann ) // Ignore non-native annotations
        return;

    QMutexLocker ml(mutex);

    Poppler::Page *ppl_page = ppl_doc->page( page );
    annotationsOnOpenHash->remove( okl_ann );
    ppl_page->removeAnnotation( ppl_ann ); // Also destroys ppl_ann
    delete ppl_page;

    okl_ann->setNativeId( QVariant::fromValue(0) ); // So that we don't double-free in disposeAnnotation

    qCDebug(OkularPdfDebug) << okl_ann->uniqueName();
}
//END PopplerAnnotationProxy implementation

Okular::Annotation* createAnnotationFromPopplerAnnotation( Poppler::Annotation *ann, bool *doDelete )
{
    Okular::Annotation *annotation = 0;
    *doDelete = true;
    bool tieToOkularAnn = false;
    bool externallyDrawn = false;
    switch ( ann->subType() )
    {
        case Poppler::Annotation::AFileAttachment:
        {
            Poppler::FileAttachmentAnnotation * attachann = static_cast< Poppler::FileAttachmentAnnotation * >( ann );
            Okular::FileAttachmentAnnotation * f = new Okular::FileAttachmentAnnotation();
            annotation = f;
            tieToOkularAnn = true;
            *doDelete = false;

            f->setFileIconName( attachann->fileIconName() );
            f->setEmbeddedFile( new PDFEmbeddedFile( attachann->embeddedFile() ) );

            break;
        }
        case Poppler::Annotation::ASound:
        {
            Poppler::SoundAnnotation * soundann = static_cast< Poppler::SoundAnnotation * >( ann );
            Okular::SoundAnnotation * s = new Okular::SoundAnnotation();
            annotation = s;

            s->setSoundIconName( soundann->soundIconName() );
            s->setSound( createSoundFromPopplerSound( soundann->sound() ) );

            break;
        }
        case Poppler::Annotation::AMovie:
        {
            Poppler::MovieAnnotation * movieann = static_cast< Poppler::MovieAnnotation * >( ann );
            Okular::MovieAnnotation * m = new Okular::MovieAnnotation();
            annotation = m;
            tieToOkularAnn = true;
            *doDelete = false;

            m->setMovie( createMovieFromPopplerMovie( movieann->movie() ) );

            break;
        }
        case Poppler::Annotation::AWidget:
        {
            annotation = new Okular::WidgetAnnotation();
            break;
        }
        case Poppler::Annotation::AScreen:
        {
            Okular::ScreenAnnotation * m = new Okular::ScreenAnnotation();
            annotation = m;
            tieToOkularAnn = true;
            *doDelete = false;
            break;
        }
#ifdef HAVE_POPPLER_0_36
        case Poppler::Annotation::ARichMedia:
        {
            Poppler::RichMediaAnnotation * richmediaann = static_cast< Poppler::RichMediaAnnotation * >( ann );
            const QPair<Okular::Movie*, Okular::EmbeddedFile*> result = createMovieFromPopplerRichMedia( richmediaann );

            if ( result.first ) {
                Okular::RichMediaAnnotation * r = new Okular::RichMediaAnnotation();
                tieToOkularAnn = true;
                *doDelete = false;
                annotation = r;

                r->setMovie( result.first );
                r->setEmbeddedFile( result.second );
            }

            break;
        }
#endif
        case Poppler::Annotation::AText:
        case Poppler::Annotation::ALine:
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
        default:
        {
            // this is uber ugly but i don't know a better way to do it without introducing a poppler::annotation dependency on core
            QDomDocument doc;
            QDomElement root = doc.createElement( QStringLiteral("root") );
            doc.appendChild( root );
            Poppler::AnnotationUtils::storeAnnotation( ann, root, doc );
            annotation = Okular::AnnotationUtils::createAnnotation( root );
            break;
        }
    }
    if ( annotation )
    {
        // the Contents field might have lines separated by \r
        QString contents = ann->contents();
        contents.replace( QLatin1Char( '\r' ), QLatin1Char( '\n' ) );

        annotation->setAuthor( ann->author() );
        annotation->setContents( contents );
        annotation->setUniqueName( ann->uniqueName() );
        annotation->setModificationDate( ann->modificationDate() );
        annotation->setCreationDate( ann->creationDate() );
        annotation->setFlags( ann->flags() | Okular::Annotation::External );
        annotation->setBoundingRectangle( Okular::NormalizedRect::fromQRectF( ann->boundary() ) );

        if (externallyDrawn)
            annotation->setFlags( annotation->flags() | Okular::Annotation::ExternallyDrawn );

        // Poppler stores highlight points in swapped order
        if ( annotation->subType() == Okular::Annotation::AHighlight )
        {
            Okular::HighlightAnnotation * hlann = static_cast<Okular::HighlightAnnotation*>( annotation );
            QList<Okular::HighlightAnnotation::Quad> &quads = hlann->highlightQuads();
            for (QList<Okular::HighlightAnnotation::Quad>::iterator it = quads.begin(); it != quads.end(); ++it)
            {
                Okular::NormalizedPoint t;
                t = it->point( 3 );
                it->setPoint( it->point(0), 3 );
                it->setPoint( t, 0 );
                t = it->point( 2 );
                it->setPoint( it->point(1), 2 );
                it->setPoint( t, 1 );
            }
        }

        if ( annotation->subType() == Okular::Annotation::AText )
        {
            Okular::TextAnnotation * txtann = static_cast<Okular::TextAnnotation*>( annotation );

            if ( txtann->textType() == Okular::TextAnnotation::Linked )
            {
                Poppler::TextAnnotation * ppl_txtann = static_cast<Poppler::TextAnnotation*>( ann );

                // Poppler and Okular assume a different default icon name in XML
                // We re-read it via getter, which always tells the right one
                txtann->setTextIcon( ppl_txtann->textIcon() );
            }
        }

        // TODO clone style
        // TODO clone window
        // TODO clone revisions
        if ( tieToOkularAnn )
        {
            annotation->setNativeId( QVariant::fromValue( ann ) );
            annotation->setDisposeDataFunction( disposeAnnotation );
        }
    }
    return annotation;
}
