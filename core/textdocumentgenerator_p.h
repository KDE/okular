/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_TEXTDOCUMENTGENERATOR_P_H_
#define _OKULAR_TEXTDOCUMENTGENERATOR_P_H_

#include <QtGui/QAbstractTextDocumentLayout>
#include <QtGui/QTextBlock>
#include <QtGui/QTextDocument>

#include "action.h"
#include "document.h"
#include "generator_p.h"
#include "textdocumentgenerator.h"

namespace Okular {

namespace TextDocumentUtils {

        static void calculateBoundingRect( QTextDocument *document, int startPosition, int endPosition,
                                           QRectF &rect, int &page )
        {
            const QSizeF pageSize = document->pageSize();

            const QTextBlock startBlock = document->findBlock( startPosition );
            const QRectF startBoundingRect = document->documentLayout()->blockBoundingRect( startBlock );

            const QTextBlock endBlock = document->findBlock( endPosition );
            const QRectF endBoundingRect = document->documentLayout()->blockBoundingRect( endBlock );

            QTextLayout *startLayout = startBlock.layout();
            QTextLayout *endLayout = endBlock.layout();

            int startPos = startPosition - startBlock.position();
            int endPos = endPosition - endBlock.position();
            const QTextLine startLine = startLayout->lineForTextPosition( startPos );
            const QTextLine endLine = endLayout->lineForTextPosition( endPos );

            double x = startBoundingRect.x() + startLine.cursorToX( startPos );
            double y = startBoundingRect.y() + startLine.y();
            double r = endBoundingRect.x() + endLine.cursorToX( endPos );
            double b = endBoundingRect.y() + endLine.y() + endLine.height();

            int offset = qRound( y ) % qRound( pageSize.height() );

            if ( x > r ) { // line break, so return a pseudo character on the start line
                rect = QRectF( x / pageSize.width(), offset / pageSize.height(),
                               3 / pageSize.width(), startLine.height() / pageSize.height() );
                page = -1;
                return;
            }

            page = qRound( y ) / qRound( pageSize.height() );
            rect = QRectF( x / pageSize.width(), offset / pageSize.height(),
                           (r - x) / pageSize.width(), (b - y) / pageSize.height() );
        }

        static void calculatePositions( QTextDocument *document, int page, int &start, int &end )
        {
            const QAbstractTextDocumentLayout *layout = document->documentLayout();
            const QSizeF pageSize = document->pageSize();
            double margin = document->rootFrame()->frameFormat().margin();

            /**
             * Take the upper left and lower left corner including the margin
             */
            start = layout->hitTest( QPointF( margin, (page * pageSize.height()) + margin ), Qt::FuzzyHit );
            end = layout->hitTest( QPointF( margin, ((page + 1) * pageSize.height()) - margin ), Qt::FuzzyHit );
        }

        static Okular::DocumentViewport calculateViewport( QTextDocument *document, const QTextBlock &block )
        {
            const QSizeF pageSize = document->pageSize();
            const QRectF rect = document->documentLayout()->blockBoundingRect( block );

            int page = qRound( rect.y() ) / qRound( pageSize.height() );
            int offset = qRound( rect.y() ) % qRound( pageSize.height() );

            Okular::DocumentViewport viewport( page );
            viewport.rePos.normalizedX = (double)rect.x() / (double)pageSize.width();
            viewport.rePos.normalizedY = (double)offset / (double)pageSize.height();
            viewport.rePos.enabled = true;
            viewport.rePos.pos = Okular::DocumentViewport::Center;

            return viewport;
        }
}

class TextDocumentConverterPrivate
{
    public:
        TextDocumentConverterPrivate()
            : mParent( 0 )
        {
        }

        TextDocumentGeneratorPrivate *mParent;
};

class TextDocumentGeneratorPrivate : public GeneratorPrivate
{
    friend class TextDocumentConverter;

    public:
        TextDocumentGeneratorPrivate( TextDocumentConverter *converter )
            : mConverter( converter ), mDocument( 0 )
        {
        }

        virtual ~TextDocumentGeneratorPrivate()
        {
            delete mConverter;
            delete mDocument;
        }

        Q_DECLARE_PUBLIC( TextDocumentGenerator )

        /* reimp */ QVariant metaData( const QString &key, const QVariant &option ) const;
        /* reimp */ QImage image( PixmapRequest * );

        void calculateBoundingRect( int startPosition, int endPosition, QRectF &rect, int &page ) const;
        void calculatePositions( int page, int &start, int &end ) const;
        Okular::TextPage* createTextPage( int ) const;

        void addAction( Action *action, int cursorBegin, int cursorEnd );
        void addAnnotation( Annotation *annotation, int cursorBegin, int cursorEnd );
        void addTitle( int level, const QString &title, const QTextBlock &position );
        void addMetaData( const QString &key, const QString &value, const QString &title );
        void addMetaData( DocumentInfo::Key, const QString &value );

        void generateLinkInfos();
        void generateAnnotationInfos();
        void generateTitleInfos();

        TextDocumentConverter *mConverter;

        QTextDocument *mDocument;
        Okular::DocumentInfo mDocumentInfo;
        Okular::DocumentSynopsis mDocumentSynopsis;

        struct TitlePosition
        {
          int level;
          QString title;
          QTextBlock block;
        };
        QList<TitlePosition> mTitlePositions;

        struct LinkPosition
        {
          int startPosition;
          int endPosition;
          Action *link;
        };
        QList<LinkPosition> mLinkPositions;

        struct LinkInfo
        {
          int page;
          QRectF boundingRect;
          Action *link;
        };
        QList<LinkInfo> mLinkInfos;

        struct AnnotationPosition
        {
          int startPosition;
          int endPosition;
          Annotation *annotation;
        };
        QList<AnnotationPosition> mAnnotationPositions;

        struct AnnotationInfo
        {
          int page;
          QRectF boundingRect;
          Annotation *annotation;
        };
        QList<AnnotationInfo> mAnnotationInfos;
};

}

#endif
