#include <kdebug.h>

#include "documentPage.h"

documentPage::documentPage() 
{
  sourceHyperLinkList.reserve(200);
  textLinkList.reserve(250);
  pageNumber = 0;
  isPixmapSet = false;
  isEmpty = true;
}


documentPage::~documentPage()
{
  ;
}


void documentPage::setPageNumber(Q_UINT16 pnr)
{
  if (pageNumber != pnr) {
    pageNumber = pnr;
    clear();
  }
}


void documentPage::clear()
{
#ifdef DEBUG_DOCUMENTPAGE
  kdDebug(4300) << "documentPage::clear() called for page #" << pageNumber << endl;
#endif

  sourceHyperLinkList.clear();
  textLinkList.clear();
  hyperLinkList.clear();
  
  isPixmapSet = false;
  isEmpty = true;
  if (!pixmap.isNull())
    pixmap.resize(0, 0);
}


QPixmap *documentPage::getPixmap()
{
  if (isPixmapSet == true)
    return &pixmap;
  else
    return 0;
}


void documentPage::setPixmap(const QPixmap &pm)
{
  pixmap = pm; 
  isPixmapSet = true;
};

#include "documentPage.moc"
