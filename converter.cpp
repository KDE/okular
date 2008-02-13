#include "converter.h"

#include <QtGui/QAbstractTextDocumentLayout>
#include <QtGui/QTextDocument>
#include <QtGui/QTextFrame>

#include <klocale.h>

#include <okular/core/action.h>


#include <QtCore/QDebug>

using namespace EPub;

Converter::Converter() : mTextDocument( 0 )
{
  
}

Converter::~Converter()
{    
  epub_close(mDocument);
  epub_cleanup();
  //  delete mTextDocument;
}

QString _strPack(xmlChar **str, int size)
{
  QString res;
  
  res = QString::fromUtf8((char *)str[0]);

  for (int i=1;i<size;i++) {
    res += ", ";
    res += QString::fromUtf8((char *)str[i]);
  }

  //  qDebug() << res << res.size();
  return res;
}

void Converter::_emitData(Okular::DocumentInfo::Key key,
                          enum epub_metadata type) 
{

  int size;
  xmlChar **data;
  
  data = epub_get_metadata(mDocument, type, &size);
  
  if (data) { 
    
    emit addMetaData(key, _strPack(data, size));
    for (int i=0;i<size;i++)
      free(data[i]);
    free(data);
  }
}

QTextDocument* Converter::convert( const QString &fileName )
{
  mTextDocument = new QTextDocument;
  mTextDocument->setPageSize( QSizeF( 600, 800 ) );
  mCursor = new QTextCursor( mTextDocument );

  QTextFrameFormat frameFormat;
  frameFormat.setMargin( 20 );
  
  QTextFrame *rootFrame = mTextDocument->rootFrame();
  rootFrame->setFrameFormat( frameFormat );

  mDocument = epub_open(qPrintable(fileName), 3);
  
  // Emit the document meta data 
  _emitData(Okular::DocumentInfo::Title, EPUB_TITLE);
  _emitData(Okular::DocumentInfo::Author, EPUB_CREATOR);
  _emitData(Okular::DocumentInfo::Subject, EPUB_SUBJECT);
  _emitData(Okular::DocumentInfo::Creator, EPUB_PUBLISHER);

  _emitData(Okular::DocumentInfo::Description, EPUB_DESCRIPTION);

  _emitData(Okular::DocumentInfo::CreationDate, EPUB_DATE);  
  _emitData(Okular::DocumentInfo::Category, EPUB_TYPE);
  _emitData(Okular::DocumentInfo::Copyright, EPUB_RIGHTS);
  emit addMetaData( Okular::DocumentInfo::MimeType, "application/epub+zip");  

  struct eiterator *it = epub_get_iterator(mDocument, EITERATOR_NONLINEAR, 0);
  
  // go in linear order
  mCursor->insertHtml(epub_it_get_curr(it));
  
  while (epub_it_get_next(it)) {
    mCursor->insertHtml(epub_it_get_curr(it));
  }

  return mTextDocument;
}
