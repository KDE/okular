#ifndef EPUB_CONVERTER_H
#define EPUB_CONVERTER_H

#include <okular/core/textdocumentgenerator.h>
#include <okular/core/document.h>

#include <epub.h>

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

      QTextDocument *mTextDocument;
      QTextCursor *mCursor;

      struct epub *mDocument;
    };
}

#endif
