#include <kdebug.h>

#include "documentPage.h"
#include "documentPageCache.h"
#include "dviwin.h"

//#define documentPageCache_DEBUG


documentPageCache::documentPageCache() 
{
  renderer = 0;
  currentPage = 0;
}


documentPageCache::~documentPageCache()
{
  delete currentPage;
}


void documentPageCache::setRenderer(dviWindow *_renderer)
{
  clear();
  renderer = _renderer;
}


documentPage *documentPageCache::getPage(Q_UINT16 pageNr)
{
#ifdef documentPageCache_DEBUG
  kdDebug(4300) << "documentPageCache::getPage( pageNr=" << pageNr << " )" << endl;
#endif

  if ((currentPage != 0) && (currentPage->getPageNumber() == pageNr) && (currentPage->isEmpty == false))
    return currentPage;

  // Allocate a currentPage structure, if necessary
  if (currentPage == 0) {
    currentPage = new documentPage();
    if (currentPage == 0) {
      kdError(4300) << "documentPageCache::getPage(..) cannot allocate documentPage structure" << endl;
      return 0;
    }
  }
  
  // Set the page number and render the page
  currentPage->setPageNumber(pageNr);
  if (renderer != 0)
    renderer->drawPage(currentPage);
  
  return currentPage;
}


void documentPageCache::clear()
{
  delete currentPage;
  currentPage = 0;
}


#include "documentPageCache.moc"
