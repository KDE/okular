#include "kpdf_canvas.h"
#include "kpdf_canvas.moc"

using namespace KPDF;

Canvas::Canvas(QWidget* parent, const char* name, WFlags f)
  : QScrollView(parent, name, f)
{
}

// vim:ts=2:sw=2:tw=78:et
