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

#include "documentPage.h"
#include "selection.h"

class QPaintEvent;
class QMouseEvent;


class documentWidget : public QWidget
{
  Q_OBJECT

public:
  documentWidget(QWidget *parent, const char *name);
  ~documentWidget();

  void setPage(documentPage *pagw);


  selection    DVIselection;


public slots:
  void          selectAll(void);
  void          copyText(void);
  void          flash(int);
  void          pixmapChanged(void);

signals:
  /** Passed through to the top-level kpart. */
  void          setStatusBarText( const QString& );
  void          localLink( const QString& );
  void          SRCLink( const QString&, QMouseEvent * e );
  void          needPixmap( documentPage * );

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

  /* This timer is used to delay clearing of the statusbar. Clearing
     the statusbar is delayed to avoid awful flickering when the mouse
     moves over a block of text that contains source hyperlinks. The
     signal timeout() is connected to the method clearStatusBar() of
     *this. */
  QTimer        clearStatusBarTimer;

  /* Data structures used for marking text with the mouse */
  QPoint       firstSelectedPoint;
  QRect        selectedRectangle;


  documentPage  *pageData;
};



#endif
