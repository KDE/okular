//
// Class: kdvi_multipage
// Author: Stefan Kebekus
//
// (C) 2001-2004, Stefan Kebekus.
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
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

//#define KDVI_MULTIPAGE_DEBUG


void KDVIMultiPage::doExportText(void)
{
#ifdef KDVI_MULTIPAGE_DEBUG
  kdDebug(4300) << "KDVIMultiPage::doExportText(void) called" << endl;
#endif

  // Paranoid safety checks
  if ((window == 0) || (window->dviFile == 0))
    return;
  if (window->dviFile->dvi_Data() == 0 )
    return;
  
  if (KMessageBox::warningContinueCancel( scrollView(),
					  i18n("<qt>This function exports the DVI file to a plain text. Unfortunately, this version of "
					       "KDVI treats only plain ASCII characters properly. Symbols, ligatures, mathematical "
					       "formulae, accented characters, and non-english text, such as Russian or Korean, will "
					       "most likely be messed up completely.</qt>"),
					  i18n("Function May Not Work as Expected"),
					  i18n("Continue Anyway"),
					  "warning_export_to_text_may_not_work") == KMessageBox::Cancel)
    return;

  // Generate a suggestion for a reasonable file name
  QString suggestedName = window->dviFile->filename;
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

  QProgressDialog progress( i18n("Exporting to text..."), i18n("Abort"), window->dviFile->total_pages, scrollView(), "export_text_progress", TRUE );
  progress.setMinimumDuration(300);

  DocumentPage dummyPage;

  for(int page=1; page <= window->dviFile->total_pages; page++) {
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
    window->drawPage(&dummyPage); // We gracefully ingore any errors (bad dvi-file, etc.) which may occur during draw_page()

    for(unsigned int i=0; i<dummyPage.textLinkList.size(); i++)
      stream << dummyPage.textLinkList[i].linkText << endl;
  }

  // Switch off the progress dialog, etc.
  progress.setProgress( window->dviFile->total_pages );
  return;
}


void  KDVIMultiPage::showFindTextDialog(void)
{
  if ((window == 0) || (window->supportsTextSearch() == false))
    return;

  if (findDialog == 0) {
    // WARNING: This text appears several times in the code. Change
    // everywhere, or nowhere!
    if (KMessageBox::warningContinueCancel( scrollView(), 
					    i18n("<qt>This function searches the DVI file for plain text. Unfortunately, this version of "
						 "KDVI treats only plain ASCII characters properly. Symbols, ligatures, mathematical "
						 "formulae, accented characters, and non-english text, such as Russian or Korean, will "
						 "most likely be messed up completely. Continue anyway?</qt>"),
					    i18n("Function May Not Work as Expected"),
					    KStdGuiItem::cont(),
					    "warning_search_text_may_not_work") == KMessageBox::Cancel)
      return;
    findDialog = new KEdFind( scrollView(), "Text find dialog", true);
    if (findDialog == 0) {
      kdError(4300) << "Could not open text search dialog" << endl;
      return;
    }
    findDialog->setName("text search dialog");
    connect(findDialog, SIGNAL(search()), this, SLOT(findText()));
  }
  findDialog->show();
}


void KDVIMultiPage::findText(void)
{
#ifdef KDVI_MULTIPAGE_DEBUG
  kdDebug(4300) << "KDVIMultiPage::findText(void) called" << endl;
#endif

  // Paranoid security checks
  if (findDialog == 0) {
    kdError(4300) << "KDVIMultiPage::findText(void) called but findDialog == 0" << endl;
    return;
  }

  QString searchText  = findDialog->getText();
  if (searchText.isEmpty())
    return;

  if (findNextAction != 0)
    findNextAction->setEnabled(!searchText.isEmpty());
  if (findPrevAction != 0)
    findPrevAction->setEnabled(!searchText.isEmpty());

  if (findDialog->get_direction())
    findPrevText();
  else
    findNextText();
}


void KDVIMultiPage::findNextText(void)
{
#ifdef KDVI_MULTIPAGE_DEBUG
  kdDebug(4300) << "KDVIMultiPage::findNextText() called" << endl;
#endif

  // Paranoia safety checks
  if (findDialog == 0) {
    kdError(4300) << "KDVIMultiPage::findNextText(void) called when findDialog == 0" << endl;
    return;
  }
  QString searchText = findDialog->getText();

  if (searchText.isEmpty()) {
    kdError(4300) << "KDVIMultiPage::findNextText(void) called when search text was empty" << endl;
    return;
  }

  bool case_sensitive = findDialog->case_sensitive();
  bool oneTimeRound   = false;


  QProgressDialog progress( i18n("Searching for '%1'...").arg(searchText), i18n("Abort"), 
			    window->dviFile->total_pages, scrollView(), "searchForwardTextProgress", TRUE );
  progress.setMinimumDuration ( 1000 );

  DocumentPage dummyPage; 

  // Find the page and text position on the page where the search will
  // start. If nothing is selected, we start at the beginning of the
  // current page. Otherwise, start after the selected text.  TODO:
  // Optimize this to get a better 'user feeling'
  Q_UINT16 startingPage;
  Q_UINT16 startingTextItem;
  if (userSelection.getPageNumber() == 0) {
    startingPage     = getCurrentPageNumber();
    startingTextItem = 0;
  } else {
    startingPage     = userSelection.getPageNumber();
    startingTextItem = userSelection.getSelectedTextEnd()+1;
  }

  for(Q_UINT16 pageNumber = startingPage;; pageNumber++) {
    // If we reach the end of the last page, start from the beginning
    // of the document, but ask the user first.
    if (pageNumber > window->dviFile->total_pages) {
      progress.hide();
      if (oneTimeRound == true)
	break;
      oneTimeRound = true;
      
      // Do not ask the user if the search really started from the
      // beginning of the first page
      if ( (startingPage == 1)&&(startingTextItem == 0) )
	return;
      
      int answ = KMessageBox::questionYesNo(scrollView(), i18n("<qt>The search string <strong>%1</strong> could not be found till the "
							       "end of the document. Should the search be restarted from the beginning "
							       "of the document?</qt>").arg(searchText), 
					    i18n("Text Not Found"));
      if (answ == KMessageBox::Yes)
	pageNumber = 1;
      else
	return;
    }
    
    if (pageNumber > startingPage)
      progress.setProgress( pageNumber - startingPage );
    else
      progress.setProgress( pageNumber +  window->dviFile->total_pages - startingPage );
    
    qApp->processEvents();
    if ( progress.wasCancelled() )
      break;
    
    dummyPage.setPageNumber(pageNumber);
    window->drawPage(&dummyPage); // We don't really care for errors in draw_page(), no error handling here.
    if (dummyPage.textLinkList.size() == 0)
      continue;
    
    // Go trough the text of the page and search for the string. How
    // much of the page we actually search depends on the page: we
    // start the search on the current page *after* the selected text,
    // if there is any, then search the other pages entirely, and then
    // the rest of the current page. 
    if (pageNumber != startingPage) {
      // On pages which are not our current page, search through
      // everything
      for(unsigned int i=0; i<dummyPage.textLinkList.size(); i++) 
	if (dummyPage.textLinkList[i].linkText.find(searchText, 0, case_sensitive) >= 0) {
	  gotoPage(pageNumber, i, i );
	  return;
	}
    } else {
      if (oneTimeRound == false) {
	// The first time we search on the current page, search
	// everything from the end of the selected text to the end of
	// the page
	for(unsigned int i=startingTextItem; i<dummyPage.textLinkList.size(); i++) 
	  if (dummyPage.textLinkList[i].linkText.find(searchText, 0, case_sensitive) >= 0) {
	    gotoPage(pageNumber, i, i );
	    return;
	  }
      } else {
	// The second time we come to the current page, search
	// everything from the beginning of the page till the
	// beginning of the selected test, then end the search.
	for(unsigned int i=0; i<startingTextItem; i++) 
	  if (dummyPage.textLinkList[i].linkText.find(searchText, 0, case_sensitive) >= 0) {
	    gotoPage(pageNumber, i, i );
	    return;
	  }
	KMessageBox::sorry( scrollView(), i18n("<qt>The search string <strong>%1</strong> could not be found.</qt>").arg(searchText) );
	return;
      }
    }
  }// of while (all pages)
}


void KDVIMultiPage::findPrevText(void)
{
#ifdef KDVI_MULTIPAGE_DEBUG
  kdDebug(4300) << "KDVIMultiPage::findPrevText() called" << endl;
#endif

  // Paranoia safety checks
  if (findDialog == 0) {
    kdError(4300) << "KDVIMultiPage::findPrevText(void) called when findDialog == 0" << endl;
    return;
  }
  QString searchText = findDialog->getText();

  if (searchText.isEmpty()) {
    kdError(4300) << "KDVIMultiPage::findPrevText(void) called when search text was empty" << endl;
    return;
  }

  bool case_sensitive = findDialog->case_sensitive();
  bool oneTimeRound    = false;


  QProgressDialog progress( i18n("Searching for '%1'...").arg(searchText), i18n("Abort"), 
			    window->dviFile->total_pages, scrollView(), "searchForwardTextProgress", TRUE );
  progress.setMinimumDuration ( 1000 );
  
  // Find the page and text position on the page where the search will
  // start. If nothing is selected, we start at the end of the current
  // page. Otherwise, start before the selected text.  TODO: Optimize
  // this to get a better 'user feeling'
  Q_UINT16 startingPage;
  Q_UINT16 startingTextItem;
  DocumentPage dummyPage; 
  if (userSelection.getPageNumber() == 0) {
    startingPage     = getCurrentPageNumber();
    dummyPage.setPageNumber(startingPage);
    window->drawPage(&dummyPage);
    startingTextItem = dummyPage.textLinkList.size();
  } else {
    startingPage     = userSelection.getPageNumber();
    startingTextItem = userSelection.getSelectedTextStart();
  }


  for(Q_UINT16 pageNumber = startingPage;; pageNumber--) {
    // If we reach the beginning of the last page, start from the end
    // of the document, but ask the user first.
    if (pageNumber <= 0) {
      progress.hide();
      if (oneTimeRound == true)
	break;
      oneTimeRound = true;

      int answ = KMessageBox::questionYesNo(scrollView(), i18n("<qt>The search string <strong>%1</strong> could not be found till the "
							       "beginning of the document. Should the search be restarted from the end "
							       "of the document?</qt>").arg(searchText), 
					    i18n("Text Not Found"));
      if (answ == KMessageBox::Yes) 
	pageNumber = window->dviFile->total_pages;
      else
	return;
    }
    
    if (pageNumber < startingPage)
      progress.setProgress( startingPage - pageNumber );
    else
      progress.setProgress( startingPage + window->dviFile->total_pages - pageNumber );

    qApp->processEvents();
    if ( progress.wasCancelled() )
      break;

    dummyPage.setPageNumber(pageNumber);
    window->drawPage(&dummyPage);
    if (dummyPage.textLinkList.size() == 0)
      continue;


    // Go trough the text of the page and search for the string. How
    // much of the page we actually search depends on the page: we
    // start the search on the current page *before* the selected
    // text, if there is any, then search the other pages entirely,
    // and then the rest of the current page.
    if (pageNumber != startingPage) {
      // On pages which are not our current page, search through
      // everything
      for(int i=dummyPage.textLinkList.size()-1; i>=0; i--) 
	if (dummyPage.textLinkList[i].linkText.find(searchText, 0, case_sensitive) >= 0) {
	  gotoPage(pageNumber, i, i );
	  return;
	}
    } else {
      if (oneTimeRound == false) {
	// The first time we search on the current page, search
	// everything from the beginning of the selected text to the
	// beginning of the page
	for(int i = startingTextItem-1; i>=0; i--) {
	  if (dummyPage.textLinkList[i].linkText.find(searchText, 0, case_sensitive) >= 0) {
	    gotoPage(pageNumber, i, i );
	    return;
	  }
	}
      } else {
	// The second time we come to the current page, search
	// everything from the end of the page till the end of the
	// selected test, then end the search.
	for(int i=dummyPage.textLinkList.size()-1; i>startingTextItem; i--)
       	  if (dummyPage.textLinkList[i].linkText.find(searchText, 0, case_sensitive) >= 0) {
	    gotoPage(pageNumber, i, i );
	    return;
	  }
	KMessageBox::sorry( scrollView(), i18n("<qt>The search string <strong>%1</strong> could not be found.</qt>").arg(searchText) );
	return;
      }
    }
  }// of while (all pages)
}
