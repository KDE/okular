// -*- C++ -*-
//
// Class: DVIWidget
//
// Widget for displaying TeX DVI files.
// Part of KDVI- A previewer for TeX DVI files.
//
// (C) 2004 Wilfried Huss. Distributed under the GPL.

#ifndef _DVIWIDGET_H_
#define _DVIWIDGET_H_

#include "documentWidget.h"

class PageView;
class DocumentPageCache;
class QPaintEvent;
class QMouseEvent;
class QWidget;
class textSelection;


class DVIWidget : public DocumentWidget
{
  Q_OBJECT

public: 
  DVIWidget(QWidget* parent, PageView* sv, DocumentPageCache* cache, const char* name);

signals:
  void SRCLink(const QString&, QMouseEvent* e, DocumentWidget*);

private:
  virtual void mousePressEvent(QMouseEvent* e);
  virtual void mouseMoveEvent(QMouseEvent* e);
};

#endif
