/***************************************************************************
 *   Copyright (C) 2005 by Stefan Kebekus                                  *
 *   kebekus@kde.org                                                       *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

#ifndef _FAXRENDERER_H_
#define _FAXRENDERER_H_


#include "documentRenderer.h"
#include "kfaximage.h"

class documentPage;

/*! \brief Well-documented minimal implementation of a documentRenderer for reading FAX files
  
This class provides a well-documented reference implementation of a
documentRenderer, suitable as a starting point for a real-world
implementation. This class is responsible for document loading and
rendering. Apart from the constructor and the descructor, it
implements only the necessary methods setFile() and drawPage().
*/

class FaxRenderer : public DocumentRenderer
{
  Q_OBJECT

public:
   /** Default constructor

       This constructor simply prints a message (if debugging is
       enabled) and calls the default constructor.
   */
   FaxRenderer(QWidget* parent);

   /** Destructor

       The destructor simpley prints a message if debugging is
       enabled. It uses the mutex to ensure that this class is not
       destructed while another thread is currently using it.
   */
  ~FaxRenderer();

  /** Opening a file

      This implementation does the necessary consistency checks and
      complains, e.g. if the file does not exist. It then uses the
      member 'fax' to load the fax file and initializes the
      appropriate data structures. The code for loading and rendering
      is contained in the class "KFaxImage", to keep this reference
      implementation short.

      @param fname the name of the file that should be opened. 
  */
  virtual bool setFile(const QString& fname);

  /** Rendering a page

      This implementation first checks if the arguments are in a
      reasonable range, and error messages are printed if this is not
      so. Secondly, the page is rendered by the KFaxImage class and
      the drawn.

      @param res resolution at which drawing should take place

      @param page pointer to a page structur on which we should draw
  */
  void drawPage(double res, RenderedDocumentPage* page);

private:
  /** This class holds the fax file */
  KFaxImage fax;
};

#endif
