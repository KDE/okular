//
// Class: DVIWidget
//
// Widget for displaying TeX DVI files.
// Part of KDVI- A previewer for TeX DVI files.
//
// (C) 2004 Wilfried Huss. Distributed under the GPL.

#ifndef _DVIWIDGET_H_
#define _DVIWIDGET_H_

#include <qwidget.h> 

#include "documentWidget.h"

class CenteringScrollview;
class documentPageCache;
class QPaintEvent;
class QMouseEvent;
class textSelection;

class DVIWidget : public documentWidget
{
  Q_OBJECT

public: 
  DVIWidget(QWidget* parent, CenteringScrollview* sv, QSize size,
            documentPageCache* cache, const char* name);
signals:
  void SRCLink(const QString&, QMouseEvent* e, documentWidget*);

private:
  virtual void mousePressEvent(QMouseEvent* e);
  virtual void mouseMoveEvent(QMouseEvent* e);
};

#endif
