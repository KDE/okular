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

#include <kdebug.h>
#include <klocale.h>
#include <core/action.h>

using namespace Epub;

Converter::Converter() : mTextDocument(NULL), mDelimiter("5937DD29BCB9BB727611E23A627DB")
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

// same as _handle_anchors, but this one iterates from the
// given start block to the end of the document, containing
// more than one html pages
void Converter::_handle_all_anchors(const QTextBlock &start) {
  mLocalLinks.clear();
  mSectionMap.clear();

  int nameIndex = 0;
  QString name = mSubDocs[nameIndex];
  for (QTextBlock bit = start; bit != mTextDocument->end(); bit = bit.next()) {
    for (QTextBlock::iterator fit = bit.begin(); !(fit.atEnd()); ++fit) {

      QTextFragment frag = fit.fragment();

      if (frag.isValid() && frag.charFormat().isAnchor()) {
        QString hrefString = frag.charFormat().anchorHref();

        // remove ./ or ../
        // making it easier to compare, with links
        while(!hrefString.isNull() && ( hrefString.at(0) == '.' || hrefString.at(0) == '/') ){
          hrefString.remove(0,1);
        }

        // if reached delimiter change prefix name
        if(hrefString == mDelimiter){
          name = mSubDocs[++nameIndex];
          if(bit.next().next().isValid())   //because bit.next() points to delimiter anchor
            mSectionMap.insert(name,bit.next().next());
        }

        QUrl href(hrefString);
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
  QString magicString = "Ub>-#+Z{DK9}2ey3Nqm4";

  mLocalLinks.clear();
  mSectionMap.clear();
  mHtmlBlocks.clear();
  mSubDocs.clear();

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
  bool first = true;
  do {
    if (epub_it_get_curr(it)) {
      QString link = QString::fromUtf8(epub_it_get_curr_url(it));
      mSubDocs.append(link);

      QString htmlContent = QString::fromUtf8(epub_it_get_curr(it));

      // adding magicStrings after every page, used while creating page breaks
      if(!first)
        htmlContent = "<p>"+magicString+"</p>" + htmlContent;
      else
        first = false;
      mHtmlBlocks.append(htmlContent);
    }
  } while (epub_it_get_next(it));


  epub_free_iterator(it);

  // preHtml & postHtml make it possible to have a margin around the content of the page
  QString preHtml = "<html><head></head><body>"
    "<table style=\"-qt-table-type: root; margin-top:20px; margin-bottom:20px; margin-left:20px; margin-right:20px;\">"
    "<tr>"
    "<td style=\"border: none;\">";
  QString postHtml = "</tr></table></body></html>";

  // a delimiter is added after every page
  // which is used in _handle_all_anchors, to change the prefix name of links
  mTextDocument->setHtml(preHtml + mHtmlBlocks.join(QString("<a href=\"%1\">&zwnj;</a>").arg(mDelimiter)) + postHtml);

  QTextCursor csr;
  int index = 0;
  // iterate over the document creating pagebreaks
  while (!(csr = mTextDocument->find(magicString,index)).isNull()) {
    QTextBlockFormat bf = csr.blockFormat();
    index = csr.position();
    bf.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
    csr.insertBlock(bf);
  }

  // handle all the links in the document
  _handle_all_anchors(mTextDocument->begin());

  // handle toc
  struct titerator *tit;

  // FIXME: support other method beside NAVMAP and GUIDE
  tit = epub_get_titerator(mTextDocument->getEpub(), TITERATOR_NAVMAP, 0);
  if (!tit)
    tit = epub_get_titerator(mTextDocument->getEpub(), TITERATOR_GUIDE, 0);

  if (tit) {
    do {
      if (epub_tit_curr_valid(tit)) {
        char *clink = epub_tit_get_curr_link(tit);
        QString link = QString::fromUtf8(clink);
        char *label = epub_tit_get_curr_label(tit);
        QTextBlock block = mTextDocument->begin(); // must point somewhere

        if (mSectionMap.contains(link))
          block = mSectionMap.value(link);

        if (block.isValid()) { // be sure we actually got a block
          emit addTitle(epub_tit_get_curr_depth(tit),
                        QString::fromUtf8(label),
                        block);
        } else {
          kDebug() << "Error: no block found for"<< link;
        }

        if (clink)
          free(clink);
        if (label)
          free(label);
      }
    } while (epub_tit_next(tit));

    epub_free_titerator(tit);
  } else {
    kDebug() << "no toc found";
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
      kDebug() << "Error: no block found for "<< hit.key();
    }
  }

  delete _cursor;

  return mTextDocument;
}
