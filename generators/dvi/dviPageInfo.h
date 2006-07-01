/***************************************************************************
 *   Copyright (C) 2006 by Luigi Toscano <luigi.toscano@tiscali.it>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _dvipageinfo_h_
#define _dvipageinfo_h_

#include "pageNumber.h"
#include "hyperlink.h"
#include "textBox.h"
#include <qvector.h>
#include <qpixmap.h>

class dviPageInfo
{

public:
   QImage img;
   int width, height;
   double resolution;
   PageNumber pageNumber;

   dviPageInfo();
   dviPageInfo( const dviPageInfo &dvipi );
   
   virtual ~dviPageInfo();

   virtual void clear();

   /** \brief List of source hyperlinks
    */
   QVector<Hyperlink> sourceHyperLinkList;

   /** \brief Hyperlinks on the document page
    */
   QVector<Hyperlink> hyperLinkList;
   QVector<TextBox> textBoxList;
};

/* quick&dirty hack to cheat the dviRenderer class... */
#define RenderedDviPagePixmap dviPageInfo
#define RenderedDocumentPagePixmap dviPageInfo
#endif
