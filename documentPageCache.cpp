//
// Class: documentPageCache
//
// Cache that holds a number of pages in a document.
// Part of KDVI- A previewer for TeX DVI files.
//
// (C) 2004 Stefan Kebekus. Distributed under the GPL.


#include <kdebug.h>

#include "documentPage.h"
#include "documentPageCache.h"
#include "dviwin.h"

//#define documentPageCache_DEBUG


documentPageCache::documentPageCache() 
{
  renderer = 0;
  LRUCache.setAutoDelete(false);
  recycleBin.setAutoDelete(false);
}


documentPageCache::~documentPageCache()
{
  LRUCache.setAutoDelete(true);
  recycleBin.setAutoDelete(true);
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

  // First check if the page that we are looking for is in the cache
  documentPage *page;
  for ( page = LRUCache.first(); page; page = LRUCache.next() )
    if ((page->getPageNumber() == pageNr) && (page->isEmpty == false)) {
      // Page found! Move the page to the front of the list, and
      // return it.
      LRUCache.remove();
      LRUCache.prepend(page);
      return page;
    }

  // The page was not found in the cache, so we have to make a new
  // page and add this to the cache. We have several options where
  // this page is supposed to come from.

  // If the cache already contains the maximum number of elements, we
  // use the last recently element, move it to the front of the list
  // and use that.
  if (LRUCache.count() == maxSize) {
    page = LRUCache.getLast();
    LRUCache.removeLast();
    page->clear();
  }

  // If we haven't found a structure in the cache, perhaps there is a
  // page structure waiting in the recycleBin that we could use?
  if ((page == 0) && (recycleBin.isEmpty() == false)) {
    page = recycleBin.first();
    recycleBin.removeFirst();
  }
  
  // If we haven't found a structure, we allocate a new one
  if (page == 0)
    page = new documentPage();
  
  // If even that failed, issue an error message and quit.
  if (page == 0) {
    kdError(4300) << "documentPageCache::getPage(..) cannot allocate documentPage structure" << endl;
    return 0;
  }
  
  // Now 'page' contains a point to a page structure that we can
  // use. Add the page as the first element to the cache, and apply
  // the renderer to the page
  LRUCache.prepend(page);
  page->setPageNumber(pageNr);
  if (renderer != 0)
    renderer->drawPage(page);

  return page;
}


void documentPageCache::clear()
{
  documentPage *page;

  while(page = LRUCache.first()) {
    LRUCache.removeFirst();
    page->clear();
    recycleBin.prepend(page);
  }
}


#include "documentPageCache.moc"
