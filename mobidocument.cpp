/***************************************************************************
 *   Copyright (C) 2008 by Jakub Stachowski <qbast@go2.pl>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include "mobidocument.h"
#include "mobipocket.h"
#include <QFile>
#include <QRegExp>
#include <kdebug.h>

using namespace Mobi;

MobiDocument::MobiDocument(const QString &fileName) : QTextDocument() 
{
  file = new QFile(fileName);
  file->open(QIODevice::ReadOnly);
  doc = new Mobipocket::Document(file);
  if (doc->isValid()) setHtml(fixMobiMarkup(doc->text()));
}

bool MobiDocument::isValid() const
{
  return doc->isValid(); 
}

MobiDocument::~MobiDocument() 
{
    delete doc;
    delete file;
}
  
QVariant MobiDocument::loadResource(int type, const QUrl &name) 
{
   
  kDebug() << "Requested resource: " << type << " URL " << name;
  
  if (type!=QTextDocument::ImageResource || name.scheme()!=QString("pdbrec")) return QVariant();
  bool ok;
  quint16 recnum=name.path().mid(1).toUShort(&ok);
  kDebug() << "Path" << name.path().mid(1) << " Img " << recnum << " all imgs " << doc->imageCount();
  if (!ok || recnum>=doc->imageCount()) return QVariant();
   
  QVariant resource;
  resource.setValue(doc->getImage(recnum));
  addResource(type, name, resource); 
    
  return resource;
}


QString MobiDocument::fixMobiMarkup(const QString& data) 
{
    QRegExp imgs("<[iI][mM][gG].*recindex=\"([0-9]*)\".*>");
    
    imgs.setMinimal(true);
    QString ret=data;
    ret.replace(imgs,"<img src=\"pdbrec:/\\1\">");
    ret.replace("<mbp:pagebreak/>","<p style=\"page-break-after:always\"></p>");
    //FIXME: anchors
    return ret;
}
