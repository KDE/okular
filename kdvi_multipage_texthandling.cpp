//
// Class: kdvi_multipage
// Author: Stefan Kebekus
//
// (C) 2001-2005, Stefan Kebekus.
//
// Previewer for TeX DVI files.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301, USA.
//
// Please report bugs or improvements, etc. via the "Report bug"-Menu
// of kdvi.

#include <config.h>

#include <kaction.h>
#include <kdebug.h>
#include <keditcl.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <qapplication.h>
#include <qprogressdialog.h>

#include "kdvi_multipage.h"
#include "dviFile.h"
#include "documentWidget.h"
#include "renderedDocumentPagePixmap.h"


//#define KDVI_MULTIPAGE_DEBUG


void KDVIMultiPage::doExportText()
{
#ifdef KDVI_MULTIPAGE_DEBUG
  kdDebug(4300) << "KDVIMultiPage::doExportText() called" << endl;
#endif

  // Paranoid safety checks
  if (DVIRenderer.dviFile == 0)
    return;
  if (DVIRenderer.dviFile->dvi_Data() == 0 )
    return;
  
  if (KMessageBox::warningContinueCancel( scrollView(),
					  i18n("<qt>This function exports the DVI file to a plain text. Unfortunately, this version of "
					       "KDVI treats only plain ASCII characters properly. Symbols, ligatures, mathematical "
					       "formulae, accented characters, and non-English text, such as Russian or Korean, will "
					       "most likely be messed up completely.</qt>"),
					  i18n("Function May Not Work as Expected"),
					  i18n("Continue Anyway"),
					  "warning_export_to_text_may_not_work") == KMessageBox::Cancel)
    return;

  KMultiPage::doExportText();
}
