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
// Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA
// 02110-1301, USA.
//
// Please report bugs or improvements, etc. via the "Report bug"-Menu
// of kdvi.


#include <kaction.h>
#include <kdebug.h>
#include <keditcl.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <qapplication.h>
#include <qprogressdialog.h>

#include "documentWidget.h"
#include "kdvi_multipage.h"
#include "renderedDocumentPagePixmap.h"


//#define KDVI_MULTIPAGE_DEBUG


void KDVIMultiPage::doExportText(void)
{
#ifdef KDVI_MULTIPAGE_DEBUG
  kdDebug(4300) << "KDVIMultiPage::doExportText(void) called" << endl;
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

  // Generate a suggestion for a reasonable file name
  QString suggestedName = DVIRenderer.dviFile->filename;
  suggestedName = suggestedName.left(suggestedName.find(".")) + ".txt";

  QString fileName = KFileDialog::getSaveFileName(suggestedName, i18n("*.txt|Plain Text (Latin 1) (*.txt)"), scrollView(), i18n("Export File As"));
  if (fileName.isEmpty())
    return;
  QFileInfo finfo(fileName);
  if (finfo.exists()) {
    int r = KMessageBox::warningYesNo (scrollView(), i18n("The file %1\nexists. Do you want to overwrite that file?").arg(fileName),
				       i18n("Overwrite File"));
    if (r == KMessageBox::No)
      return;
  }

  QFile textFile(fileName);
  textFile.open( IO_WriteOnly );
  QTextStream stream( &textFile );

  QProgressDialog progress( i18n("Exporting to text..."), i18n("Abort"), DVIRenderer.totalPages(), scrollView(), "export_text_progress", TRUE );
  progress.setMinimumDuration(300);

  RenderedDocumentPagePixmap dummyPage;
  dummyPage.resize(1,1);

  for(int page=1; page <= DVIRenderer.totalPages(); page++) {
    progress.setProgress( page );
    // Funny. The manual to QT tells us that we need to call
    // qApp->processEvents() regularly to keep the application from
    // freezing. However, the application crashes immediately if we
    // uncomment the following line and works just fine as it is. Wild
    // guess: Could that be related to the fact that we are linking
    // agains qt-mt?

    // qApp->processEvents();

    if ( progress.wasCancelled() )
      break;

    dummyPage.setPageNumber(page);
    DVIRenderer.drawPage(100.0, &dummyPage); // We gracefully ingore any errors (bad dvi-file, etc.) which may occur during draw_page()

    for(unsigned int i=0; i<dummyPage.textBoxList.size(); i++)
      stream << dummyPage.textBoxList[i].text << endl;
  }

  // Switch off the progress dialog, etc.
  progress.setProgress( DVIRenderer.totalPages() );
  return;
}
