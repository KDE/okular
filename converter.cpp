/***************************************************************************
 *   Copyright (C) 2008 by Jakub Stachowski <qbast@go2.pl>                 *
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
#include <QtCore/QFile>
#include <mobipocket.h>

#include <klocale.h>
#include <okular/core/action.h>

using namespace Mobi;

Converter::Converter() 
{
  
}

Converter::~Converter()
{    
}


QTextDocument* Converter::convert( const QString &fileName )
{
  MobiDocument* newDocument=new MobiDocument(fileName);
  if (!newDocument->isValid()) {
    emit error(i18n("Error while opening the Mobipocket document."), -1);
    delete newDocument;
    return NULL;
  }
  newDocument->setPageSize(QSizeF(600, 800));

  QTextFrameFormat frameFormat;
  frameFormat.setMargin( 20 );
  QTextFrame *rootFrame = newDocument->rootFrame();
  rootFrame->setFrameFormat( frameFormat ); 
  return newDocument;
}
