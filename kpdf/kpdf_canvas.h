#ifndef _KPDF_CANVAS_H_
#define _KPDF_CANVAS_H_

#include <qscrollview.h>

namespace KPDF
{
  class Canvas : public QScrollView
  {
    Q_OBJECT

  public:
    Canvas(QWidget* parent = 0, const char* name = 0, WFlags f = 0);
  };
}

#endif

// vim:ts=2:sw=2:tw=78:et
