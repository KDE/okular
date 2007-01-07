/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef FICTIONBOOK_CONVERTER_H
#define FICTIONBOOK_CONVERTER_H

#include <QtCore/QDateTime>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCharFormat>
#include <QtXml/QDomDocument>

#include <okular/core/document.h>

class QDomElement;
class QDomText;

namespace FictionBook {

class Document;

class Converter
{
    public:
        Converter( const Document *document );
        ~Converter();

        bool convert();

        QTextDocument *textDocument() const;

        Okular::DocumentInfo documentInfo() const;
        Okular::DocumentSynopsis tableOfContents() const;

        class LinkInfo
        {
            public:
                typedef QList<LinkInfo> List;

                int page;
                QRectF boundingRect;
                QString url;
        };

        LinkInfo::List links();

    private:
        bool convertBody( const QDomElement &element );
        bool convertDescription( const QDomElement &element );
        bool convertSection( const QDomElement &element );
        bool convertTitle( const QDomElement &element );
        bool convertParagraph( const QDomElement &element );
        bool convertBinary( const QDomElement &element );
        bool convertCover( const QDomElement &element );
        bool convertImage( const QDomElement &element );
        bool convertEpigraph( const QDomElement &element );
        bool convertPoem( const QDomElement &element );
        bool convertSubTitle( const QDomElement &element );
        bool convertCite( const QDomElement &element );
        bool convertEmptyLine( const QDomElement &element );
        bool convertLink( const QDomElement &element );
        bool convertEmphasis( const QDomElement &element );
        bool convertStrong( const QDomElement &element );
        bool convertStyle( const QDomElement &element );


        bool convertTitleInfo( const QDomElement &element );
        bool convertDocumentInfo( const QDomElement &element );
        bool convertAuthor( const QDomElement &element,
                            QString &firstName, QString &middleName, QString &lastName,
                            QString &email, QString &nickname );
        bool convertDate( const QDomElement &element, QDate &date );
        bool convertTextNode( const QDomElement &element, QString &data );

        void calculateBoundingRect( int, int, QRectF&, int& );
        void createLinkInfos();

        const Document *mDocument;
        QTextDocument *mTextDocument;
        QTextCursor *mCursor;

        class TitleInfo;
        TitleInfo *mTitleInfo;

        class DocumentInfo;
        DocumentInfo *mDocumentInfo;

        struct HeaderInfo
        {
            QTextBlock block;
            QString text;
            int level;
        };
        QList<HeaderInfo> mHeaderInfos;

        struct LinkPosition
        {
            int startPosition;
            int endPosition;
            QString url;
        };

        QList<LinkPosition> mLinkPositions;
        LinkInfo::List mLinkInfos;

        bool mLinkInfosGenerated;

        int mSectionCounter;
};

}

#endif
