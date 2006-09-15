/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OOO_STYLEPARSER_H
#define OOO_STYLEPARSER_H

class QDomElement;

namespace OOO {

class Document;
class StyleInformation;

class StyleParser
{
  public:
    StyleParser( const Document *document, StyleInformation *styleInformation );

    bool parse();

  private:
    bool parseContentFile();
    bool parseStyleFile();
    bool parseMetaFile();

    bool parseDocumentCommonAttrs( QDomElement& );
    bool parseFontFaceDecls( QDomElement& );
    bool parseStyles( QDomElement& );
    bool parseMasterStyles( QDomElement& );
    bool parseAutomaticStyles( QDomElement& );

    StyleFormatProperty parseStyleProperty( QDomElement& );
    ParagraphFormatProperty parseParagraphProperty( QDomElement& );
    TextFormatProperty parseTextProperty( QDomElement& );
    PageFormatProperty parsePageProperty( QDomElement& );

    double convertUnit( const QString& ) const;

    const Document *mDocument;
    StyleInformation *mStyleInformation;
};

}

#endif
