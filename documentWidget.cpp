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

#include "documentWidget.h"


documentWidget::documentWidget(QWidget *parent, const char *name )
  : QWidget( parent, name )
{
  // Variables used in animation.
  animationCounter = 0;
  timerIdent       = 0;

  pageData         = 0;

  DVIselection.clear();
  setMouseTracking(true);
  resize(100,100);
}


documentWidget::~documentWidget()
{
  ;
}


void documentWidget::pixmapChanged(void)
{
  /*
  // Stop any animation which may be in progress
  if (timerIdent != 0) {
  killTimer(timerIdent);
  timerIdent       = 0;
  animationCounter = 0;
  }
  */
  
  // Remove the mouse selection
  DVIselection.clear();
  
  if (pageData != 0) {
    if (pageData->getPixmap() != 0)
      resize(pageData->getPixmap()->size());
  } else
    resize(0,0);
  update();
}


void documentWidget::setPage(documentPage *page)
{
  pageData = page;
  if (pageData != 0) 
    connect(page, SIGNAL(pixmapChanged()), this, SLOT(pixmapChanged()));
  pixmapChanged();
}


void documentWidget::timerEvent( QTimerEvent *e )
{
  if (pageData == 0)
    return;

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
  if (pageData == 0)
    return;

  if (pageData->getPixmap() == 0) 
    emit(needPixmap(pageData));
  else {
    bitBlt ( this, e->rect().topLeft(), pageData->getPixmap(), e->rect(), CopyROP);
    
    QPainter p(this);
    p.setClipRect(e->rect());
    
    if (animationCounter > 0 && animationCounter < 10) {
      int wdt = width()/(10-animationCounter);
      int hgt = height()/((10-animationCounter)*20);
      p.setPen(QPen(QColor(150,0,0), 3, DashLine));
      p.drawRect((width()-wdt)/2, flashOffset, wdt, hgt);
    }
    
    // Mark selected text.
    if (DVIselection.selectedTextStart != -1)
      for(unsigned int i = DVIselection.selectedTextStart; (i <= DVIselection.selectedTextEnd)&&(i < pageData->textLinkList.size()); i++) {
	p.setPen( NoPen );
	p.setBrush( white );
	p.setRasterOp( Qt::XorROP );
	p.drawRect(pageData->textLinkList[i].box);
      }
  }

}


void documentWidget::copyText(void)
{
  QApplication::clipboard()->setSelectionMode(false);
  QApplication::clipboard()->setText(DVIselection.selectedText);
}


void documentWidget::selectAll(void)
{
  if (pageData == 0)
    return;

  QString selectedText("");
  for(unsigned int i = 0; i < pageData->textLinkList.size(); i++) {
    selectedText += pageData->textLinkList[i].linkText;
    selectedText += "\n";
  }
  DVIselection.set(0, pageData->textLinkList.size()-1, selectedText);
  update();
}


void documentWidget::mousePressEvent ( QMouseEvent * e )
{
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "mouse event" << endl;
#endif

  if (pageData == 0)
    return;

  // Check if the mouse is pressed on a regular hyperlink
  if (e->button() == LeftButton) {
    if (pageData->hyperLinkList.size() > 0)
      for(int i=0; i<pageData->hyperLinkList.size(); i++) {
	if (pageData->hyperLinkList[i].box.contains(e->pos())) {
	  emit(localLink(pageData->hyperLinkList[i].linkText));
	  return;
	}
      }
    setCursor(Qt::SizeAllCursor);
  }
  
  // Check if the mouse is pressed on a source-hyperlink
  if ((e->button() == MidButton) && (pageData->sourceHyperLinkList.size() > 0))
    for(unsigned int i=0; i<pageData->sourceHyperLinkList.size(); i++)
      if (pageData->sourceHyperLinkList[i].box.contains(e->pos())) {
	emit(SRCLink(pageData->sourceHyperLinkList[i].linkText, e));
	return;
      }
}


void documentWidget::mouseReleaseEvent ( QMouseEvent * )
{
  unsetCursor();
  selectedRectangle.setRect(0,0,0,0);
}


void documentWidget::mouseMoveEvent ( QMouseEvent * e )
{
  if (pageData == 0)
    return;

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

    if ((selectedTextStart != DVIselection.selectedTextStart) || (selectedTextEnd != DVIselection.selectedTextEnd)) {
      if (selectedTextEnd == -1) {
	DVIselection.clear();
	update();
      } else {
	// Find the rectangle that needs to be updated (reduces
	// flicker)
	int a = DVIselection.selectedTextStart;
	int b = DVIselection.selectedTextEnd+1;
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
	DVIselection.set(selectedTextStart, selectedTextEnd, selectedText);
	update(box);
      }
    }
  }
}


#include "documentWidget.moc"
