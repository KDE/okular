//
// Class: documentPageCache
//
// Cache that holds a number of pages in a document.
// Part of KDVI- A previewer for TeX DVI files.
//
// (C) 2004 Stefan Kebekus. Distributed under the GPL.


#ifndef _documentpagecache_h_
#define _documentpagecache_h_

#include <qobject.h>
#include <qpixmap.h>
#include <qrect.h>
#include <qstring.h>
#include <qvaluevector.h>

class documentPage;
class dviWindow;


class documentPageCache: public QObject
{
 Q_OBJECT

 public:
  documentPageCache();
  ~documentPageCache();

  void     setRenderer(dviWindow *_renderer);
  Q_UINT16 getPageNumber();

  /** Returns a pointer to a documentPage structure, or 0, if the
      documentPage could not be generated for some reason. */
  documentPage *getPage(Q_UINT16 pageNr);
  

 public slots:
  /** Clears the contents of the cache. */
  void clear();

 private:
  documentPage *currentPage;
  Q_UINT16      currentPageNr;
  dviWindow    *renderer;
};


#endif
