//
// Class: documentWidget
//
// Widget for displaying TeX DVI files.
// Part of KDVI- A previewer for TeX DVI files.
//
// (C) 2001 Stefan Kebekus
// Distributed under the GPL
//

#include <kdebug.h>
#include <klocale.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qcursor.h>
#include <qpainter.h>

#include "../kviewshell/centeringScrollview.h"
#include "documentPage.h"
#include "documentPageCache.h"
#include "documentWidget.h"
#include "selection.h"

//#define DEBUG_DOCUMENTWIDGET


documentWidget::documentWidget(QWidget *parent, CenteringScrollview *sv, QSize size, documentPageCache *cache, selection *documentSelection, const char *name )
  : QWidget( parent, name )
{
  // Variables used in animation.
  animationCounter = 0;
  timerIdent       = 0;
  documentCache    = cache;
  DVIselection     = documentSelection;
  scrollView       = sv;

  setMouseTracking(true);
  resize(size);

  setBackgroundMode(Qt::NoBackground);
}


void documentWidget::setPageNumber(Q_UINT16 nr)
{
  pageNr = nr;
  update();
}


void documentWidget::timerEvent( QTimerEvent *e )
{
  animationCounter++;
  if (animationCounter >= 10) {
    killTimer(e->timerId());
    timerIdent       = 0;
    animationCounter = 0;
  }
  repaint(0, flashOffset, width(), height()/19, false);
}


void documentWidget::flash(int fo)
{
  animationCounter = 0;
  if (timerIdent != 0)
    killTimer(timerIdent);
  flashOffset      = fo;
  timerIdent       = startTimer(50); // Start the animation. The animation proceeds in 1/10s intervals
}


void documentWidget::paintEvent(QPaintEvent *e)
{
#ifdef DEBUG_DOCUMENTWIDGET
  kdDebug(4300) << "documentWidget::paintEvent() called" << endl;
#endif

  // Check if this widget is really visible to the user. If not, there
  // is nothing to do. Remark: if we don't do this, then under QT
  // 3.2.3 the following happens: when the user changes the zoom
  // value, all those widgets are updated which the user has EVER
  // seen, not just those that are visible at the moment. If the
  // document contains several thousand pages, it is easily possible
  // that this means that a few hundred of the are re-painted (which
  // takes substantial time) although only three widgets are visible
  // and *should* be upadated. I believe this is some error in QT, but
  // I am not positive about that ---Stefan Kebekus.
  QRect visiblRect(scrollView->contentsX(), scrollView->contentsY(), scrollView->visibleWidth(), scrollView->visibleHeight());
  QRect widgetRect(scrollView->childX(this), scrollView->childY(this), width(), height() );
  if (!widgetRect.intersects(visiblRect))
    return;


  documentPage *pageData = documentCache->getPage(pageNr);
  if (pageData == 0) {
#ifdef DEBUG_DOCUMENTWIDGET
    kdDebug(4300) << "documentWidget::paintEvent: no documentPage generated" << endl;
#endif
    return;
  }

  QPixmap *pixmap = pageData->getPixmap();
  if (pixmap == 0) {
#ifdef DEBUG_DOCUMENTWIDGET
    kdDebug(4300) << "documentWidget::paintEvent: no pixmap found" << endl;
#endif
    return;
  }

  // Resize the widget, if appropriate
  if (pixmap->size() != this->size()) {
    resize(pixmap->size());
    // emit the signal 'resized' so that the widget can be newly
    // centered
    emit resized();
  }
  
  // Paint widget contents
  bitBlt ( this, e->rect().topLeft(), pageData->getPixmap(), e->rect(), CopyROP);
  
  // Paint flashing frame, if appropriate
  QPainter p(this);
  p.setClipRect(e->rect());
  if (animationCounter > 0 && animationCounter < 10) {
    int wdt = width()/(10-animationCounter);
    int hgt = height()/((10-animationCounter)*20);
    p.setPen(QPen(QColor(150,0,0), 3, DashLine));
    p.drawRect((width()-wdt)/2, flashOffset, wdt, hgt);
  }
  
  // Mark selected text.
  
  if ((DVIselection->page != 0)&&(DVIselection->page == pageNr)) {
    // @@@ Add paranoid safety check about beginning and end 
    if (DVIselection->selectedTextStart != -1)
      for(unsigned int i = DVIselection->selectedTextStart; (i <= DVIselection->selectedTextEnd)&&(i < pageData->textLinkList.size()); i++) {
	p.setPen( NoPen );
	p.setBrush( white );
	p.setRasterOp( Qt::XorROP );
	p.drawRect(pageData->textLinkList[i].box);
      }
  }
}


void documentWidget::selectAll(void)
{
  kdDebug(4300) << "documentWidget::selectAll(void) called" << endl;

  // pageNr == 0 indicated an invalid page (e.g. page number not yet
  // set)
  if (pageNr == 0)
    return;

  // Get a pointer to the page contents
  documentPage *pageData = documentCache->getPage(pageNr);
  if (pageData == 0) {
    kdDebug(4300) << "documentWidget::selectAll() pageData for page #" << pageNr << " is empty" << endl;
    return;
  }
  
  // mark everything as selected
  QString selectedText("");
  for(unsigned int i = 0; i < pageData->textLinkList.size(); i++) {
    selectedText += pageData->textLinkList[i].linkText;
    selectedText += "\n";
  }
  Q_UINT16 oldPageNr = DVIselection->page;
  DVIselection->set(pageNr, 0, pageData->textLinkList.size()-1, selectedText);
  if (oldPageNr != pageNr)
    connect(DVIselection, SIGNAL(pageChanged(void)), this, SLOT(selectionPageChanged(void)));

  // Re-paint
  update();
}


void documentWidget::mousePressEvent ( QMouseEvent * e )
{
#ifdef DEBUG_DOCUMENTWIDGET
  kdDebug(4300) << "documentWidget::mousePressEvent(...) called" << endl;
#endif
  
  // Make sure the event is passed on to the higher-level widget;
  // otherwise QT gets the coordinated in the mouse move events wrong
  e->ignore();

  // pageNr == 0 indicated an invalid page (e.g. page number not yet
  // set)
  if (pageNr == 0)
    return;

  // Get a pointer to the page contents
  documentPage *pageData = documentCache->getPage(pageNr);
  if (pageData == 0) {
    kdDebug(4300) << "documentWidget::selectAll() pageData for page #" << pageNr << " is empty" << endl;
    return;
  }

  // Check if the mouse is pressed on a regular hyperlink
  if (e->button() == LeftButton) {
    if (pageData->hyperLinkList.size() > 0)
      for(int i=0; i<pageData->hyperLinkList.size(); i++) {
	if (pageData->hyperLinkList[i].box.contains(e->pos())) {
	  emit(localLink(pageData->hyperLinkList[i].linkText));
	  e->accept();
	  return;
	}
      }
    setCursor(Qt::SizeAllCursor);
  }
  
  // Check if the mouse is pressed on a source-hyperlink
  if ((e->button() == MidButton) && (pageData->sourceHyperLinkList.size() > 0))
    for(unsigned int i=0; i<pageData->sourceHyperLinkList.size(); i++)
      if (pageData->sourceHyperLinkList[i].box.contains(e->pos())) {
	emit(SRCLink(pageData->sourceHyperLinkList[i].linkText, e, this));
	e->accept();
	return;
      }
  
  if (e->button() == RightButton)
  {
    setCursor(Qt::IbeamCursor);
    DVIselection->clear();
  }
}


void documentWidget::mouseReleaseEvent ( QMouseEvent *e )
{
  // Make sure the event is passed on to the higher-level widget;
  // otherwise the mouse cursor in the centeringScrollview is wrong
  e->ignore();

  unsetCursor();
  selectedRectangle.setRect(0,0,0,0);
}


void documentWidget::mouseMoveEvent ( QMouseEvent * e )
{
#ifdef DEBUG_DOCUMENTWIDGET
  kdDebug(4300) << "documentWidget::mouseMoveEvent(...) called" << endl;
#endif


  // pageNr == 0 indicated an invalid page (e.g. page number not yet
  // set)
  if (pageNr == 0)
    return;

  // Get a pointer to the page contents
  documentPage *pageData = documentCache->getPage(pageNr);
  if (pageData == 0) {
    kdDebug(4300) << "documentWidget::selectAll() pageData for page #" << pageNr << " is empty" << endl;
    return;
  }

  // If no mouse button pressed
  if ( e->state() == 0 ) {
    // go through hyperlinks
    for(unsigned int i=0; i<pageData->hyperLinkList.size(); i++) {
      if (pageData->hyperLinkList[i].box.contains(e->pos())) {
	clearStatusBarTimer.stop();
	setCursor(pointingHandCursor);
	QString link = pageData->hyperLinkList[i].linkText;
	if ( link.startsWith("#") )
	  link = link.remove(0,1);
	emit setStatusBarText( i18n("Link to %1").arg(link) );
	return;
      }
    }

    // Cursor not over hyperlink? Then let the cursor be the usual arrow
    setCursor(arrowCursor);

    // But maybe the cursor hovers over a sourceHyperlink?
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
    if (!clearStatusBarTimer.isActive())
      clearStatusBarTimer.start( 200, TRUE ); // clear the statusbar after 200 msec.
    return;
  }

  // Left mouse button pressed -> Text copy function
  if ((e->state() & LeftButton) != 0) {
    // Pass the mouse event on to the owner of this widget ---under
    // normal circumstances that is the centeringScrollView which will
    // then scroll the scrollview contents
    e->ignore();
  }

  // Right mouse button pressed -> Text copy function
  if ((e->state() & RightButton) != 0) {
    if (selectedRectangle.isEmpty()) {
      firstSelectedPoint = e->pos();
      selectedRectangle.setRect(e->pos().x(),e->pos().y(),1,1);
    } else {
      int lx = e->pos().x() < firstSelectedPoint.x() ? e->pos().x() : firstSelectedPoint.x();
      int rx = e->pos().x() > firstSelectedPoint.x() ? e->pos().x() : firstSelectedPoint.x();
      int ty = e->pos().y() < firstSelectedPoint.y() ? e->pos().y() : firstSelectedPoint.y();
      int by = e->pos().y() > firstSelectedPoint.y() ? e->pos().y() : firstSelectedPoint.y();
      selectedRectangle.setCoords(lx,ty,rx,by);
    }

    // Now that we know the rectangle, we have to find out which words
    // intersect it!
    Q_INT32 selectedTextStart = -1;
    Q_INT32 selectedTextEnd   = -1;

    for(unsigned int i=0; i<pageData->textLinkList.size(); i++)
      if ( selectedRectangle.intersects(pageData->textLinkList[i].box) ) {
	if (selectedTextStart == -1)
	  selectedTextStart = i;
	selectedTextEnd = i;
      }

    QString selectedText("");


    if (selectedTextStart != -1)
      for(unsigned int i = selectedTextStart; (i <= selectedTextEnd)&&(i < pageData->textLinkList.size()); i++) {
	selectedText += pageData->textLinkList[i].linkText;
	selectedText += "\n";
      }

    if ((selectedTextStart != DVIselection->selectedTextStart) || (selectedTextEnd != DVIselection->selectedTextEnd)) {
      if (selectedTextEnd == -1) {
	DVIselection->clear();
	update();
      } else {
	// Find the rectangle that needs to be updated (reduces
	// flicker)
	int a = DVIselection->selectedTextStart;
	int b = DVIselection->selectedTextEnd+1;
	int c = selectedTextStart;
	int d = selectedTextEnd+1;

	int i1 = kMin(a,c);
	int i2 = kMin(kMax(a,c),kMin(b,d));
	int i3 = kMax(kMax(a,c),kMin(b,d));
	int i4 = kMax(b,d);

	QRect box;
	int i=i1;
	while(i<i2) {
	  if (i != -1)
	    box = box.unite(pageData->textLinkList[i].box);
	  i++;
	}
	
	for(int i=i3; i<i4; i++)
	  if (i != -1)
	    box = box.unite(pageData->textLinkList[i].box);
	
	Q_UINT16 oldPageNr = DVIselection->page;
	DVIselection->set(pageNr, selectedTextStart, selectedTextEnd, selectedText);
	if (oldPageNr != pageNr)
	  connect(DVIselection, SIGNAL(pageChanged(void)), this, SLOT(selectionPageChanged(void)));
	
	update(box);
      }
    }
  }
}


void  documentWidget::selectionPageChanged(void)
{
  update();
  disconnect(DVIselection);
}

#include "documentWidget.moc"
