//
// Class: documentWidet
//
// Widget for displaying TeX DVI files.
// Part of KDVI- A previewer for TeX DVI files.
//
// (C) 2004 Stefan Kebekus. Distributed under the GPL.


#ifndef _dviwidget_h_
#define _dviwidget_h_

#include <qtimer.h> 
#include <qwidget.h> 

class CenteringScrollview;
class documentPageCache;
class QPaintEvent;
class QMouseEvent;
class selection;

class documentWidget : public QWidget
{
  Q_OBJECT

public:
  documentWidget(QWidget *parent, CenteringScrollview *sv, QSize size, documentPageCache *cache, selection *documentSelection, const char *name);


  void          setPageNumber(Q_UINT16 pageNr);
  Q_UINT16      getPageNumber() {return pageNr;};


public slots:
  void          selectAll(void);
  void          flash(int);

  /** This slot is called by the 'selection', when text in a different
      page is selected */
  void          selectionPageChanged(void);


signals:
  /** Passed through to the top-level kpart. */
  void          setStatusBarText( const QString& );
  void          localLink( const QString& );
  void          SRCLink(const QString&, QMouseEvent * e, documentWidget *);

  /** This signal is emitted when the widget resizes itself */
  void          resized(void);

protected:
  void          paintEvent (QPaintEvent *);

private:
  void          mousePressEvent ( QMouseEvent * e );
  void          mouseReleaseEvent (QMouseEvent *);
  void          mouseMoveEvent (QMouseEvent *);

  /** Methods and counters used for the animation to mark the target of
      an hyperlink. */
  int           timerIdent;
  void          timerEvent( QTimerEvent *e );
  int           animationCounter;
  int           flashOffset;

  Q_UINT16      pageNr;

  /* This timer is used to delay clearing of the statusbar. Clearing
     the statusbar is delayed to avoid awful flickering when the mouse
     moves over a block of text that contains source hyperlinks. The
     signal timeout() is connected to the method clearStatusBar() of
     *this. */
  QTimer        clearStatusBarTimer;

  /* Data structures used for marking text with the mouse */
  QPoint       firstSelectedPoint;
  QRect        selectedRectangle;

  /** Pointer to the CenteringScrollview that contains this
      widget. This pointer is used in the re-implementation of the
      paintEvent() method ---see the explanation there. */
  CenteringScrollview *scrollView;
  documentPageCache *documentCache;
  selection *DVIselection;
};



#endif
