/***************************************************************************
 *   Copyright (C) 2008 by Jakub Stachowski <qbast@go2.pl>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include "okular/mobidocument.h"
#include "lib/mobipocket.h"
#include "lib/qfilestream.h"
#include <QtCore/QFile>
#include <QtCore/QRegExp>
#include <kdebug.h>

using namespace Mobi;

MobiDocument::MobiDocument(const QString &fileName) : QTextDocument() 
{
  file = new Mobipocket::QFileStream(fileName);
  doc = new Mobipocket::Document(file);
  if (doc->isValid()) setHtml(fixMobiMarkup(doc->text()));
}

MobiDocument::~MobiDocument() 
{
    delete doc;
    delete file;
}
  
QVariant MobiDocument::loadResource(int type, const QUrl &name) 
{
  if (type!=QTextDocument::ImageResource || name.scheme()!=QString("pdbrec")) return QVariant();
  bool ok;
  quint16 recnum=name.path().mid(1).toUShort(&ok);
  if (!ok || recnum>=doc->imageCount()) return QVariant();
   
  QVariant resource;
  resource.setValue(doc->getImage(recnum-1));
  addResource(type, name, resource); 
    
  return resource;
}

// starting from 'pos', find position in the string that is not inside a tag
int outsideTag(const QString& data, int pos)
{
  for (int i=pos-1;i>=0;i--) {
    if (data[i]=='>') return pos;
    if (data[i]=='<') return i;
  }
  return pos;
}

QString MobiDocument::fixMobiMarkup(const QString& data) 
{
    QString ret=data;
    QMap<int,QString> anchorPositions;
    static QRegExp anchors("<a(?: href=\"[^\"]*\"){0,1}[\\s]+filepos=['\"]{0,1}([\\d]+)[\"']{0,1}", Qt::CaseInsensitive);
    int pos=0;

    // find all link destinations
    while ( (pos=anchors.indexIn( data, pos ))!=-1) {
	int filepos=anchors.cap( 1 ).toUInt(  );
	if (filepos) anchorPositions[filepos]=anchors.cap(1);
	pos+=anchors.matchedLength();
    }

    // put HTML anchors in all link destinations
    int offset=0;
    QMapIterator<int,QString> it(anchorPositions);
    while (it.hasNext()) {
      it.next();
      // link pointing outside the document, ignore
      if ( (it.key()+offset) >= ret.size()) continue;
      int fixedpos=outsideTag(ret, it.key()+offset);
      ret.insert(fixedpos,QString("<a name=\"")+it.value()+QString("\">&nbsp;</a>"));
      // inserting anchor shifts all offsets after the anchor
      offset+=21+it.value().size();
    }

    // replace links referencing filepos with normal internal links
    ret.replace(anchors,"<a href=\"#\\1\"");
    // Mobipocket uses strange variang of IMG tags: <img recindex="3232"> where recindex is number of 
    // record containing image
    static QRegExp imgs("<img.*recindex=\"([\\d]*)\".*>", Qt::CaseInsensitive);
    
    imgs.setMinimal(true);
    ret.replace(imgs,"<img src=\"pdbrec:/\\1\">");
    ret.replace("<mbp:pagebreak/>","<p style=\"page-break-after:always\"></p>");
    return ret;
}
