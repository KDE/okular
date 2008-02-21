/***************************************************************************
 *   Copyright (C) 2008 by Ely Levy <elylevy@cs.huji.ac.il>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

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
  epub_cleanup();
  //  delete mTextDocument;
}

QString _strPack(unsigned char **str, int size)
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
  unsigned char **data;
  
  data = epub_get_metadata(mTextDocument->getEpub(), type, &size);
  
  if (data) { 
    
    emit addMetaData(key, _strPack(data, size));
    for (int i=0;i<size;i++)
      free(data[i]);
    free(data);
  }
}

QTextDocument* Converter::convert( const QString &fileName )
{
  mTextDocument = new EpubDocument(fileName);
  mTextDocument->setPageSize(QSizeF(600, 800));

  mCursor = new QTextCursor( mTextDocument );

  QTextFrameFormat frameFormat;
  frameFormat.setMargin( 20 );
  
  QTextFrame *rootFrame = mTextDocument->rootFrame();
  rootFrame->setFrameFormat( frameFormat );

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

  struct eiterator *it;

  // iterate over the book
  it = epub_get_iterator(mTextDocument->getEpub(), EITERATOR_SPINE, 0);

  do {
    if (epub_it_get_curr(it)) {
      mCursor->insertBlock();
      //      emit addTitle(level, heading, mCursor->block());  
      mCursor->insertHtml(epub_it_get_curr(it));
      
      int page = mTextDocument->pageCount();
      while(mTextDocument->pageCount() == page)
        mCursor->insertText("\n");
    }
  } while (epub_it_get_next(it));

  epub_free_iterator(it);

  return mTextDocument;
}
