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
#include <QDir>

#include <kdebug.h>
#include <klocale.h>
#include <core/action.h>

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
  const QString curDir = QFileInfo(name).path();

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

        QUrl href(hrefString);
        if (href.isValid() && !href.isEmpty()) {
          if (href.isRelative()) { // Inside document link
            if(!hrefString.indexOf('#'))
              hrefString = name + hrefString;
            else if(QFileInfo(hrefString).path() == "." && curDir != ".")
              hrefString = curDir + QDir::separator() + hrefString;

            // QTextCharFormat sometimes splits a link in two
            // if there's no white space between words & the first one is an anchor
            // consider whole word to be an anchor
            ++fit;
            int fragLen = frag.length();
            if(!fit.atEnd() && ((fit.fragment().position() - frag.position()) == 1))
              fragLen += fit.fragment().length();
            --fit;

            _insert_local_links(hrefString,
                                QPair<int, int>(frag.position(),
                                                frag.position()+fragLen));
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

void Converter::_insert_local_links(const QString &key, const QPair<int, int> &value)
{
  if(mLocalLinks.contains(key)){
    mLocalLinks[key].append(value);
  } else {
    QVector< QPair<int, int> > vec;
    vec.append(value);
    mLocalLinks.insert(key,vec);
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

  QTextCursor *_cursor = new QTextCursor( mTextDocument );

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

  // if the background color of the document is non-white it will be handled by QTextDocument::setHtml()
  bool firstPage = true;
  do{
    if(epub_it_get_curr(it)) {
      const QString link = QString::fromUtf8(epub_it_get_curr_url(it));
      mTextDocument->setCurrentSubDocument(link);
      QString htmlContent = QString::fromUtf8(epub_it_get_curr(it));

      // as QTextCharFormat::anchorNames() ignores sections, replace it with <p>
      htmlContent.replace(QRegExp("< *section"),"<p");
      htmlContent.replace(QRegExp("< */ *section"),"</p");

      // convert svg tags to img
      static const int maxHeight = mTextDocument->maxContentHeight();
      static const int maxWidth = mTextDocument->maxContentWidth();
      QDomDocument dom;
      if(dom.setContent(htmlContent)) {
        QDomNodeList svgs = dom.elementsByTagName("svg");
        if(!svgs.isEmpty()) {
          QList< QDomNode > imgNodes;
          for (int i = 0; i < svgs.length(); ++i) {
            QDomNodeList images = svgs.at(i).toElement().elementsByTagName("image");
            for (int j = 0; j < images.length(); ++j) {
              QString lnk = images.at(i).toElement().attribute("xlink:href");
              int ht = images.at(i).toElement().attribute("height").toInt();
              int wd = images.at(i).toElement().attribute("width").toInt();
              QImage img = mTextDocument->loadResource(QTextDocument::ImageResource,QUrl(lnk)).value<QImage>();
              if(ht == 0) ht = img.height();
              if(wd == 0) wd = img.width();
              if(ht > maxHeight) ht = maxHeight;
              if(wd > maxWidth) wd = maxWidth;
              mTextDocument->addResource(QTextDocument::ImageResource,QUrl(lnk),img);
              QDomDocument newDoc;
              newDoc.setContent(QString("<img src=\"%1\" height=\"%2\" width=\"%3\" />").arg(lnk).arg(ht).arg(wd));
              imgNodes.append(newDoc.documentElement());
            }
            foreach (QDomNode nd, imgNodes) {
              svgs.at(i).parentNode().replaceChild(nd,svgs.at(i));
            }
          }
          htmlContent = dom.toString();
        }
      }

      QTextBlock before;
      const QString css = "<style> body { color : "+ mTextDocument->txtColor.name()+"; }"
        " a { color : blue; }</style>";
      if(firstPage) {
        // preHtml & postHtml make it possible to have a margin around the content of the page
        const QString preHtml = QString("<html><head>"+ css +"</head><body>"
                                        "<table style=\"-qt-table-type: root; margin-top:%1px; margin-bottom:%1px; margin-left:%1px; margin-right:%1px;\">"
                                        "<tr>"
                                        "<td style=\"border: none;\">").arg(mTextDocument->padding);
        const QString postHtml = "</tr></table></body></html>";
        mTextDocument->setHtml(preHtml + htmlContent + postHtml);
        firstPage = false;
        before = mTextDocument->begin();
      } else {
        before = _cursor->block();
        _cursor->insertHtml(css + htmlContent);
      }

      mSectionMap.insert(link, before);

      _handle_anchors(before, link);

      const int page = mTextDocument->pageCount();

      // it will clear the previous format
      // useful when the last line had a bullet
      _cursor->insertBlock(QTextBlockFormat());

      while(mTextDocument->pageCount() == page)
        _cursor->insertText("\n");
    }
  } while (epub_it_get_next(it));

  epub_free_iterator(it);

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
  QHashIterator<QString, QVector< QPair<int, int> > > hit(mLocalLinks);
  while (hit.hasNext()) {
    hit.next();

    const QTextBlock block = mSectionMap.value(hit.key());

    for (int i = 0; i < hit.value().size(); ++i) {
      if (block.isValid()) { // be sure we actually got a block
        Okular::DocumentViewport viewport =
          calculateViewport(mTextDocument, block);

        Okular::GotoAction *action = new Okular::GotoAction(QString(), viewport);

        emit addAction(action, hit.value()[i].first, hit.value()[i].second);
      } else {
        kDebug() << "Error: no block found for "<< hit.key();
      }
    }
  }

  delete _cursor;

  return mTextDocument;
}
