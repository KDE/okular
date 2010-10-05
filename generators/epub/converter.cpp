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

        if (href.isValid() && !href.isEmpty()) {
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
        }
        
        const QStringList &names = frag.charFormat().anchorNames();
        if (!names.empty()) {
          for (QStringList::const_iterator lit = names.constBegin(); 
               lit != names.constEnd(); ++lit) {
            mSectionMap.insert(name + '#' + *lit, bit);
          }
        }
        
      } // end anchor case
    }
  }
}

QTextDocument* Converter::convert( const QString &fileName )
{
  EpubDocument *newDocument = new EpubDocument(fileName);
  if (!newDocument->isValid()) {
    emit error(i18n("Error while opening the EPub document."), -1);
    delete newDocument;
    return NULL;
  }
  mTextDocument = newDocument;

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

      QString link = QString::fromUtf8(epub_it_get_curr_url(it));
      mTextDocument->setCurrentSubDocument(link);

      // Pass on all the anchor since last block
      const QTextBlock &before = _cursor->block();
      mSectionMap.insert(link, before);
      _cursor->insertHtml(QString::fromUtf8(epub_it_get_curr(it)));

      // Add anchors to hashes
      _handle_anchors(before, link);

      // Start new file in a new page 
      int page = mTextDocument->pageCount();
      while(mTextDocument->pageCount() == page)
        _cursor->insertText("\n");
    }
  } while (epub_it_get_next(it));

  epub_free_iterator(it);
  mTextDocument->setCurrentSubDocument(QString());

  
  // handle toc
  struct titerator *tit;
  
  // FIXME: support other method beside NAVMAP and GUIDE
  tit = epub_get_titerator(mTextDocument->getEpub(), TITERATOR_NAVMAP, 0);
  if (! tit) 
    tit = epub_get_titerator(mTextDocument->getEpub(), TITERATOR_GUIDE, 0);

  if (tit) {
    do {
      if (epub_tit_curr_valid(tit)) {
        char *clink = epub_tit_get_curr_link(tit);
        QString link = QString::fromUtf8(clink);
        char *label = epub_tit_get_curr_label(tit);
        QTextBlock block = mTextDocument->begin(); // must point somewhere
        
        if(mSectionMap.contains(link)) {
          block = mSectionMap.value(link);
        } else { // load missing resource
          char *data;
          int size = epub_get_data(mTextDocument->getEpub(), clink, &data);
          if (size > 0) {
            _cursor->insertBlock();

            // try to load as image and if not load as html
            block = _cursor->block();
            QImage image;
            mSectionMap.insert(link, block);
            if (image.loadFromData((unsigned char *)data, size)) {
              mTextDocument->addResource(QTextDocument::ImageResource, 
                                         QUrl(link), image); 
              _cursor->insertImage(link);
            } else {
              _cursor->insertHtml(QString::fromUtf8(data));
              // Add anchors to hashes
              _handle_anchors(block, link);
            }

            // Start new file in a new page 
            int page = mTextDocument->pageCount();
            while(mTextDocument->pageCount() == page)
              _cursor->insertText("\n");
          }
            
          free(data);
        }
        
        if (block.isValid()) { // be sure we actually got a block
          emit addTitle(epub_tit_get_curr_depth(tit),
                        QString::fromUtf8(label),
                        block);
          
        } else {
          qDebug() << "Error: no block found for "<< link << "\n";
        }
                 
        if (clink)
          free(clink);
        
        if (label)
          free(label);
      }
    } while (epub_tit_next(tit));

    epub_free_titerator(tit);
  } else {
    qDebug() << "no toc found\n";
  }

  // adding link actions
  QHashIterator<QString, QPair<int, int> > hit(mLocalLinks);
  while (hit.hasNext()) {
    hit.next();

    const QTextBlock block = mSectionMap.value(hit.key());
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
