// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
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
class QPoint;
class QWidget;
class textSelection;


class DVIWidget : public DocumentWidget
{
  Q_OBJECT

public:
  DVIWidget(PageView* sv, DocumentPageCache* cache, const char* name);

signals:
  void SRCLink(const QString&, const QPoint&, DocumentWidget*);

private:
  virtual void mousePressEvent(QMouseEvent* e);
  virtual void mouseMoveEvent(QMouseEvent* e);
};

#endif
