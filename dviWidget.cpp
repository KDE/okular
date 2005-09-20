//
// Class: DVIWidget
//
// Widget for displaying TeX DVI files.
// Part of KDVI- A previewer for TeX DVI files.
//
// (C) 2004 Wilfried Huss, Stefan Kebekus
// Distributed under the GPL
//

#include <kdebug.h>
#include <klocale.h>

#include "dviWidget.h"

#include "documentPageCache.h"
#include "documentWidget.h"
#include "hyperlink.h"
#include "pageView.h"
#include "renderedDviPagePixmap.h"
#include "selection.h"

DVIWidget::DVIWidget(QWidget* parent, PageView* sv, DocumentPageCache* cache, const char* name)
  : DocumentWidget(parent, sv, cache, name)
{
}


void DVIWidget::mousePressEvent(QMouseEvent* e)
{
  // Call implementation from parent
  DocumentWidget::mousePressEvent(e);

  // pageNr == 0 indicated an invalid page (e.g. page number not yet set)
  if (pageNr == 0)
    return;

  // Get a pointer to the page contents
  RenderedDviPagePixmap* pageData = dynamic_cast<RenderedDviPagePixmap*>(documentCache->getPage(pageNr));
  if (pageData == 0)
  {
    kdDebug(4300) << "DVIWidget::mousePressEvent(...) pageData for page #" << pageNr << " is empty" << endl;
    return;
  }

  // Check if the mouse is pressed on a source-hyperlink
  if ((e->button() == MidButton) && (pageData->sourceHyperLinkList.size() > 0))
  {
    for(unsigned int i=0; i<pageData->sourceHyperLinkList.size(); i++)
    {
      if (pageData->sourceHyperLinkList[i].box.contains(e->pos()))
      {
        emit(SRCLink(pageData->sourceHyperLinkList[i].linkText, e, this));
        e->accept();
        return;
      }
    }
  }
}


void DVIWidget::mouseMoveEvent(QMouseEvent* e)
{
  // pageNr == 0 indicated an invalid page (e.g. page number not yet set)
  if (pageNr == 0)
    return;

  // Call the standard implementation
  DocumentWidget::mouseMoveEvent(e);
  
  // Analyze the mouse movement only if no mouse button was pressed
  if ( e->state() == 0 ) {
    // Get a pointer to the page contents
    RenderedDviPagePixmap* pageData = dynamic_cast<RenderedDviPagePixmap*>(documentCache->getPage(pageNr));
    if (pageData == 0) {
      kdDebug(4300) << "DVIWidget::mouseMoveEvent(...) pageData for page #" << pageNr << " is empty" << endl;
      return;
    }
    
    // Check if the cursor hovers over a sourceHyperlink.
    for(unsigned int i=0; i<pageData->sourceHyperLinkList.size(); i++) {
      if (pageData->sourceHyperLinkList[i].box.contains(e->pos())) {
	clearStatusBarTimer.stop();
	
	// The macro-package srcltx gives a special like "src:99 test.tex"
	// while MikTeX gives "src:99test.tex". KDVI tries
	// to understand both.
	QString cp = pageData->sourceHyperLinkList[i].linkText;
	int max = cp.length();
	int i;
	for(i=0; i<max; i++)
	  if (cp[i].isDigit() == false)
	    break;
	
	emit setStatusBarText( i18n("line %1 of %2").arg(cp.left(i)).arg(cp.mid(i).simplifyWhiteSpace()) );
	return;
      }
    }
  }
}


#include "dviWidget.moc"
