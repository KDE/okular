/***************************************************************************
 *   Copyright (C) 2008 by Ely Levy <elylevy@cs.huji.ac.il>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef EPUB_CONVERTER_H
#define EPUB_CONVERTER_H

#include <okular/core/textdocumentgenerator.h>
#include <okular/core/document.h>

#include "epubdocument.h"

class QDomElement;
class QTextCursor;


namespace EPub {
  class Converter : public Okular::TextDocumentConverter
    {
    public:
      Converter();
      ~Converter();
      
      virtual QTextDocument *convert( const QString &fileName );
      
    private:
      
      void _emitData(Okular::DocumentInfo::Key key, enum epub_metadata type); 

      EpubDocument *mTextDocument;
      QTextCursor *mCursor;
      
    };
}

#endif
