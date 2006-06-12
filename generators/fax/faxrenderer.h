/***************************************************************************
 *   Copyright (C) 2005 by Stefan Kebekus <kebekus@kde.org>                *
 *   Copyright (C) 2005 by Piotr Szymański <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef _FAXRENDERER_H_
#define _FAXRENDERER_H_

#include <qmutex.h>
#include "core/generator.h"
#include "kfaximage.h"

/*! \brief Well-documented minimal implementation of a Generator for reading FAX files
  
This class provides a well-documented reference implementation of a
Generator, suitable as a starting point for a real-world
implementation. This class is responsible for providing abstract layer of okular
with all the needed document information.
*/

class FaxRenderer : public Generator
{
  Q_OBJECT

public:
   /** Default constructor

       This constructor simply prints a message (if debugging is
       enabled) and calls the default constructor.
   */
   FaxRenderer(KPDFDocument * doc);

   /** Destructor

       The destructor simpley prints a message if debugging is
       enabled. It uses the mutex to ensure that this class is not
       destructed while another thread is currently using it.
   */
//   ~FaxRenderer();

  /** Opening a file

      This function does what is needed to parse the file, which 
      was already checked for consistency and complains. It populates
      the pages vector with relevant information.

      @param fileName the name of the file that should be opened. 
      @param pagesVector the vector of pages with information about their sizes and rotation
      @return returns true if the document was loaded, false if not
  */
  bool loadDocument( const QString & fileName, QVector< KPDFPage * > & pagesVector );

  /** Rendering a page

      This implementation takes care of generating a page requested in PixmapRequest.

      The code for loading and rendering is contained in the class "KFaxImage", 
      to keep this reference implementation short.

      @param request pointer to the request which contains information about the page that is to be rendered

  */
  void generatePixmap( PixmapRequest * request );
  bool canGeneratePixmap( bool async ) { return !mutex.locked(); };

private:
  /** This class holds the fax file */
    KFaxImage fax;
    QMutex mutex;
};

#endif
