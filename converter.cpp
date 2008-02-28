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
#include <QTextDocumentFragment>
#include <QtCore/QDebug>

#include <klocale.h>
#include <okular/core/action.h>

using namespace Epub;

Converter::Converter() : mTextDocument(NULL)
{
  
}

Converter::~Converter()
{    
  epub_cleanup();
}

// join the char * array into one QString
QString _strPack(char **str, int size)
{
  QString res;
  
  res = QString::fromUtf8(str[0]);

  for (int i=1;i<size;i++) {
    res += ", ";
    res += QString::fromUtf8(str[i]);
  }

  return res;
}

// emit data wrap function that map between epub metadata to okular's
void Converter::_emitData(Okular::DocumentInfo::Key key,
                          enum epub_metadata type) 
{

  int size;
  unsigned char **data;
  
  data = epub_get_metadata(mTextDocument->getEpub(), type, &size);
  
  if (data) { 
    
    emit addMetaData(key, _strPack((char **)data, size));
    for (int i=0;i<size;i++)
      free(data[i]);
    free(data);
  }
}

// Got over the blocks from start and add them to hashes use name as the 
// prefix for local links
void Converter::_handle_anchors(const QTextBlock &start, const QString &name) {

  for (QTextBlock bit = start; bit != mTextDocument->end(); bit = bit.next()) {
    for (QTextBlock::iterator fit = bit.begin(); !(fit.atEnd()); ++fit) {
      
      QTextFragment frag = fit.fragment();
      
      if (frag.isValid() && frag.charFormat().isAnchor()) {
        QUrl href(frag.charFormat().anchorHref());

        if (href.isValid() && ! href.isEmpty())
          if (href.isRelative()) { // Inside document link
            mLocalLinks.insert(href.toString(), 
                               QPair<int, int>(frag.position(), 
                                               frag.position()+frag.length()));
          } else { // Outside document link       
            Okular::BrowseAction *action = 
              new Okular::BrowseAction(href.toString());
            
            emit addAction(action, frag.position(), 
                           frag.position() + frag.length());
          }
        
        const QStringList &names = frag.charFormat().anchorNames();
        if (!names.empty()) {
          for (QStringList::const_iterator lit = names.constBegin(); 
               lit != names.constEnd(); ++lit) {
            mSectionMap.insert(name + "#" + *lit, bit);
          }
        }
        
      } // end anchor case
    }
  }
}

QTextDocument* Converter::convert( const QString &fileName )
{
  mTextDocument = new EpubDocument(fileName);
  if (!mTextDocument->isValid()) {
    emit error("Error opening document", -1);
    return NULL;
  }

  mTextDocument->setPageSize(QSizeF(600, 800));

  QTextCursor *_cursor = new QTextCursor( mTextDocument );

  QTextFrameFormat frameFormat;
  frameFormat.setMargin( 20 );
  
  QTextFrame *rootFrame = mTextDocument->rootFrame();
  rootFrame->setFrameFormat( frameFormat );

  mLocalLinks.clear();
  mSectionMap.clear();

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
      
      // insert block for links
      _cursor->insertBlock();

      QString link(epub_it_get_curr_url(it));

      // Pass on all the anchor since last block
      const QTextBlock &before = _cursor->block();
      mSectionMap.insert(link, before);
      _cursor->insertHtml(epub_it_get_curr(it));

      // Add anchors to hashes
      _handle_anchors(before, link);

      // Start new file in a new page 
      int page = mTextDocument->pageCount();
      while(mTextDocument->pageCount() == page)
        _cursor->insertText("\n");
    }
  } while (epub_it_get_next(it));

  epub_free_iterator(it);

  // adding link actions
  QHashIterator<QString, QPair<int, int> > hit(mLocalLinks);
  while (hit.hasNext()) {
    hit.next();

    const QTextBlock &block = mSectionMap[hit.key()];
    if (block.isValid()) { // be sure we actually got a block
      Okular::DocumentViewport viewport = 
        calculateViewport(mTextDocument, block);

      Okular::GotoAction *action = new Okular::GotoAction(QString(), viewport);
    
      emit addAction(action, hit.value().first, hit.value().second);
    } else {
      qDebug() << "Error: no block found for "<< hit.key() << "\n";
    }
  }

  delete _cursor;

  return mTextDocument;
}
