/***************************************************************************
 *   Copyright (C) 2008 by Ely Levy <elylevy@cs.huji.ac.il>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef EPUB_DOCUMENT_H
#define EPUB_DOCUMENT_H

#include <QTextDocument>
#include <QImage>
#include <epub.h>

namespace EPub {
  class EpubDocument : public QTextDocument 
  {
    
  public:
  
    
    EpubDocument(const QString &fileName) : QTextDocument() {
      mEpub = epub_open(qPrintable(fileName), 3);
    }
  
    ~EpubDocument() {
      epub_close(mEpub);
    }
  
    struct epub *getEpub() {
      return mEpub;
    }
    
  protected:
    virtual QVariant loadResource(int type, const QUrl &name) {
      int size;
      char *data;
      //    qDebug() << "loading resource: type -" << type << "name -" << name.toString() << "\n";
    
      size = epub_get_data(mEpub, qPrintable(name.toString()), &data);
      QVariant resource;
    
      switch(type) {
      case QTextDocument::ImageResource:
        resource.setValue(QImage::fromData((unsigned char *)data, size));
        break;
      
      default:
        resource.setValue(QString(data));
        break;
      }
    
      free(data);
    
      // add to cache
      addResource(type, name, resource); 
    
      return resource;
    }
    
  private:
    struct epub *mEpub;
  };
}
#endif
