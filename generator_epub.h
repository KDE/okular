#ifndef _OKULAR_GENERATOR_EPUB_H_
#define _OKULAR_GENERATOR_EPUB_H_
#include <okular/core/textdocumentgenerator.h>

class EPubGenerator : public Okular::TextDocumentGenerator
{
 public:
  EPubGenerator( QObject *parent, const QVariantList &args );
  ~EPubGenerator() {};
  
  /*        MagicGenerator( QObject *parent, const QVariantList &args );
            ~MagicGenerator();
            
            bool loadDocument( const QString &fileName, QVector<Okular::Page*> &pages );
            
            bool canGeneratePixmap() const;
            void generatePixmap( Okular::PixmapRequest *request );
            
            protected:
            bool doCloseDocument();
            
            private:
            MagicDocument mMagicDocument;
  */
};

#endif
