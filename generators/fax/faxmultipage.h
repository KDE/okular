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

#ifndef __FAXMULTIPAGE_H
#define __FAXMULTIPAGE_H

#include <qstringlist.h>

#include "kmultipage.h"
#include "faxrenderer.h"



/*! \mainpage FaxMultiPage
  
\section intro_sec Introduction

FaxMultiPage is a minimal, but well-documented reference
implementation of a kviewshell plugin that can serve as a starting
point for implementations.

\section install_sec Usage

When FaxMultiPage and the associated files are installed, the
kviewshell program can open TIFF-Fax anf G3 fax files, i.e. files of
mime types image/fax-g3 or image/tiff.
 
\section content Content

Only the two classes that are absolutely necessary for a working
plugin are implemented. The only other file that is installed are
desktop file, which tells kviewshell to use the plugin.

- FaxMultiPage, an implementation of a KMultiPage. In a larger
application, this class would contain the GUI elements that the plugin
adds to the GUI of the kviewshell. For viewing FAXes, no special GUI
elements are required, and this plugin does only the minimal
initialization required.

- FaxRenderer, an implementation of a DocumentRenderer. This class is
responsible for document loading and rendering.

- kfaxmultipage.desktop and kfaxmultipage_tiff.desktop are desktop
entry files that associate the plugin wit image/fax-g3 or image/tiff
mime-types.  Without these files installed, the file dialog in
kviewshell would not show FAX files, and the command line "kvieshell
test.g3" would fail with an error dialog "No plugin for image/fax-g3
files installed".
*/



 
/*! \brief Well-documented minimal implementation of a KMultiPage
  
This class provides a well-documented reference implementation of a
KMultiPage, suitable as a starting point for a real-world
implementation. In a larger application, this class would contain the
GUI elements that the plugin adds to the GUI of the kviewshell. For
viewing FAXes, no special GUI elements are required, and this plugin
does only the minimal initialization required.
*/

class FaxMultiPage : public KMultiPage
{
  Q_OBJECT

public:
  /** Constructor
      
  The constructor needs to initialize several members of the
  kmultipage. Please have a look at the constructor's source code to
  see how to adjust this for your implementation.
  */
  FaxMultiPage(QWidget *parentWidget, const char *widgetName, QObject *parent,
		   const char *name, const QStringList& args = QStringList());
  
  /** Destructor
		
  This destructor does nothing.
  */
  virtual ~FaxMultiPage();

  /** List of file formats for file saving
  
  This method returns the list of supported file formats for saving
  the file.
  */
  virtual QStringList fileFormats();

  /** Author information
 
  This member returns a structure that contains information about the
  authors of the implementation
  */
  static KAboutData* createAboutData();
  
 private:
  /** This member holds the renderer which is used by the demo implementation */
  FaxRenderer     faxRenderer;
};

#endif
