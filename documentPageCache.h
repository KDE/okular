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

  /** This method is used to name the documentRenderer, that the
      documentPageCache uses to render the page. The renderer can be
      used any time (e.g., triggered by an internal timer event), and
      must not be deleted before either the documentRenderer is
      deleted, or another renderer has been set. */
  void     setRenderer(dviWindow *_renderer);

  /** Returns a pointer to a documentPage structure, or 0, if the
      documentPage could not be generated for some reason (e.g.,
      because no renderer has been set). */
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
