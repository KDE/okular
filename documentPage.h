//
// Class: documentPage
//
// Widget for displaying TeX DVI files.
// Part of KDVI- A previewer for TeX DVI files.
//
// (C) 2004 Stefan Kebekus. Distributed under the GPL.


#ifndef _documentpage_h_
#define _documentpage_h_

#include <qobject.h>
#include <qpixmap.h>
#include <qrect.h>
#include <qstring.h>
#include <qvaluevector.h>



class DVI_Hyperlink {
 public:
  DVI_Hyperlink() {}
  DVI_Hyperlink(Q_UINT32 bl, QRect re, QString lT): baseline(bl), box(re), linkText(lT) {}

  Q_UINT32 baseline;
  QRect    box;
  QString  linkText;
};



// This class contains everything one needs dviwin needs to know about
// a certain page. 

class documentPage: public QObject
{
 Q_OBJECT

 public:
  documentPage();
  ~documentPage();

  /** This method sets the number of the page that this instance
      represents. A value of '0' means that this class does not
      represent any page ('invalid page'). This method also calls
      clear(). */
  void setPageNumber(Q_UINT16 pagenr);

  /** This method sets the number of the page that this instance
      represents. A value of '0' means that this class does not
      represent any page ('invalid page'). This method also calls
      clear(). */
  Q_UINT16 getPageNumber() {return pageNumber;};

  /** Returns a pointer to the pixmap last set, or 0 if no pixmap has
      been set since the class has been constructed or last
      clear()ed. */
  QPixmap *getPixmap();

  void setPixmap(const QPixmap &pm);
  
  // List of source-hyperlinks in the current page. This vector is
  // generated when the current page is drawn.
  QValueVector<DVI_Hyperlink> sourceHyperLinkList;

  QValueVector<DVI_Hyperlink> textLinkList; // List of text in the window
  QValueVector<DVI_Hyperlink> hyperLinkList; // List of ordinary hyperlinks

 public slots:
  /** Clears the contents of the class, but leaves pageNumber intact,
      and does not free the memory for the QValueVectors so the
      lengthy re-allocation won't be necessary later. Clears the
      Pixmap. Before setPixmap() is called next time, getPixmap() will
      return 0. */
  void clear();


 signals:
  void      pixmapChanged(void);

 private:
  QPixmap   pixmap;
  bool      isPixmapSet;

  /** Number of the page that this instance represents a value of '0'
      means that this class does not represent any page, i.e. that a
      page number has not been set yet. */
  Q_UINT16   pageNumber;
};


#endif
