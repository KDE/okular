
//
// Class: dviWindow
//
// Previewer for TeX DVI files.
//

#include <qapplication.h> 
#include <qpainter.h>
#include <qprogressdialog.h> 
#include <kmessagebox.h>
#include <kdebug.h>
#include <keditcl.h>
#include <klocale.h>


#include "dviwin.h"

extern QPainter foreGroundPaint; // QPainter used for text

void dviWindow::showFindTextDialog(void)
{
  if (findDialog == 0) {
    // WARNING: This text appears several times in the code. Change
    // everywhere, or nowhere!
    if (KMessageBox::warningContinueCancel( this, 
					    i18n("<qt>This function searches the DVI file for plain text. Unfortunately, this version of "
						 "KDVI treats only plain ASCII characters properly. Symbols, ligatures, mathematical "
						 "formulae, accented characters, and non-english text, such as Russian or Korean, will "
						 "most likely be messed up completely. Continue anyway?</qt>"),
					    i18n("Function may not work as expected"),
					    i18n("Continue"),
					    "warning_search_text_may_not_work") == KMessageBox::Cancel)
      return;
    findDialog = new KEdFind(this, "Text find dialog", true);
    connect(findDialog, SIGNAL(search()), this, SLOT(findText()));
  }
  findDialog->show();
}

void dviWindow::findText(void)
{
  searchText  = findDialog->getText();
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

void dviWindow::findNextText(void)
{
  if (findDialog == 0) {
    kdError(4300) << "findNextText called when findDialog == 0" << endl;
    return;
  }

  if (searchText.isEmpty()) {
    kdError(4300) << "findNextText called when search text was empty" << endl;
    return;
  }

  bool case_sensitive = findDialog->case_sensitive();

  bool _postscript_sav = _postscript;
  bool oneTimeRound    = false;
  int current_page_sav = current_page;

  unsigned int firstPageOfSearch = current_page;
  unsigned int lastPageOfSearch  = dviFile->total_pages-1;

  _postscript = FALSE; // Switch off postscript to speed up things...
  QPixmap pixie(1,1); // Dummy pixmap for the method draw_page which wants to have a valid painter. 

  QProgressDialog progress( i18n("Searching for '%1'...").arg(searchText), i18n("Abort"), 
			    lastPageOfSearch - firstPageOfSearch, this, "searchForwardTextProgress", TRUE );
  progress.setMinimumDuration ( 1000 );
  while(current_page <= lastPageOfSearch) {
    progress.setProgress( current_page - firstPageOfSearch );
    qApp->processEvents();
    if ( progress.wasCancelled() )
      break;

    // Go trough the text of the current page and search for the
    // string.
    for(int i=DVIselection.selectedTextStart+1; i<num_of_used_textlinks; i++) 
      if (textLinkList[i].linkText.find(searchText, 0, case_sensitive) >= 0) {
	// Restore the previous settings, including the current
	// page. Otherwise, the program is "smart enough" not to
	// re-render the screen.
	_postscript    = _postscript_sav;
	int j = current_page;
	current_page   = current_page_sav;
	emit(request_goto_page(j, textLinkList[i].box.bottom() ));
	DVIselection.set(i, i, textLinkList[i].linkText);
	repaint();
	return;
      }
    DVIselection.clear();
    current_page++;

    if ((current_page == dviFile->total_pages)) {
      progress.hide();
      if (oneTimeRound == true)
	break;
      oneTimeRound = true;
      if (current_page_sav != 0) {
	int answ = KMessageBox::questionYesNo(this, i18n("<qt>The search string <strong>%1</strong> could not be found till the "
							 "end of the document. Should the search be restarted from the beginning "
							 "of the document?</qt>").arg(searchText), 
					      i18n("Text not found")); 
	if (answ == KMessageBox::Yes) {
	  firstPageOfSearch = 0;
	  current_page      = 0;
	  lastPageOfSearch  = current_page_sav;
	  progress.setTotalSteps(lastPageOfSearch);
	  progress.setProgress(0);
	}
      }
    }
    foreGroundPaint.begin( &pixie );
    draw_page(); // We don't really care for errors in draw_page(), no error handling here.
    foreGroundPaint.end();
  }// of while (all pages)
  
  KMessageBox::sorry( this, i18n("<qt>The search string <strong>%1</strong> could not be found.</qt>").arg(searchText) );

  // Restore the PostScript setting 
  _postscript = _postscript_sav;
  
  // Restore the current page.
  current_page = current_page_sav;
  foreGroundPaint.begin( &pixie );
  draw_page(); // We don't care for errors here
  foreGroundPaint.end();
}


void dviWindow::findPrevText(void)
{
  if (findDialog == 0) {
    kdError(4300) << "findNextText called when findDialog == 0" << endl;
    return;
  }

  if (searchText.isEmpty()) {
    kdError(4300) << "findNextText called when search text was empty" << endl;
    return;
  }

  bool case_sensitive = findDialog->case_sensitive();

  bool _postscript_sav = _postscript;
  bool oneTimeRound    = false;
  int current_page_sav = current_page;

  unsigned int firstPageOfSearch = 0;
  unsigned int lastPageOfSearch  = current_page;

  _postscript = FALSE; // Switch off postscript to speed up things...
  QPixmap pixie(1,1); // Dummy pixmap for the method draw_page which wants to have a valid painter. 

  QProgressDialog progress( i18n("Searching for '%1'...").arg(searchText), i18n("Abort"), 
			    lastPageOfSearch - firstPageOfSearch, this, "searchForwardTextProgress", TRUE );
  progress.setMinimumDuration ( 1000 );

  if (DVIselection.selectedTextStart == 0) {
    current_page--;
    if (current_page >= dviFile->total_pages) { // Note: current_page is unsigned. It will not become negative, but very big
      oneTimeRound      = true;
      firstPageOfSearch = current_page_sav;
      current_page      = dviFile->total_pages-1;
      lastPageOfSearch  = dviFile->total_pages-1;
      progress.setTotalSteps(lastPageOfSearch-firstPageOfSearch);
      progress.setProgress(0);
      DVIselection.clear();
      foreGroundPaint.begin( &pixie );
      draw_page(); // We don't really care for errors in draw_page(), no error handling here.
      foreGroundPaint.end();
    }
  }

  do {
    if ((current_page >= dviFile->total_pages)) { // Note: current_page is unsigned. It will not become negative, but very big
      progress.hide();
      if (oneTimeRound == true)
	break;
      oneTimeRound = true;
      if (current_page_sav != 0) {
	int answ = KMessageBox::questionYesNo(this, i18n("<qt>The search string <strong>%1</strong> could not be found till the "
							 "beginning of the document. Should the search be restarted from the end "
							 "of the document?</qt>").arg(searchText), 
					      i18n("Text not found")); 
	if (answ == KMessageBox::Yes) {
	  firstPageOfSearch = current_page_sav;
	  current_page      = dviFile->total_pages-1;
	  lastPageOfSearch  = dviFile->total_pages-1;
	  progress.setTotalSteps(lastPageOfSearch-firstPageOfSearch);
	  progress.setProgress(0);
	}
      }
    }


    progress.setProgress(  lastPageOfSearch - current_page );
    qApp->processEvents();
    if ( progress.wasCancelled() )
      break;
    
    // Go trough the text of the current page and search for the
    // string.
    int i=DVIselection.selectedTextStart-1;
    if (i < 0)
      i = num_of_used_textlinks;
    do{
      if (textLinkList[i].linkText.find(searchText, 0, case_sensitive) >= 0) {
	// Restore the previous settings, including the current
	// page. Otherwise, the program is "smart enough" not to
	// re-render the screen.
	_postscript    = _postscript_sav;
	int j = current_page;
	current_page   = current_page_sav;
	emit(request_goto_page(j, textLinkList[i].box.bottom() ));
	DVIselection.set(i, i, textLinkList[i].linkText);
	repaint();
	return;
      }
      i--;
    }while(i >= 0);
    current_page--;
    DVIselection.clear();
    foreGroundPaint.begin( &pixie );
    draw_page(); // We don't really care for errors in draw_page(), no error handling here.
    foreGroundPaint.end();
  } while( current_page < dviFile->total_pages ); // Note: current_page is unsigned. It will not become negative, but very big
  
  KMessageBox::sorry( this, i18n("<qt>The search string <strong>%1</strong> could not be found.</qt>").arg(searchText) );

  // Restore the PostScript setting 
  _postscript = _postscript_sav;
  
  // Restore the current page.
  current_page = current_page_sav;
  foreGroundPaint.begin( &pixie );
  draw_page(); // We don't care for errors here
  foreGroundPaint.end();
}
