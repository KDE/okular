// -*- C++ -*-

//#define PERFORMANCE_MEASUREMENT

#ifdef PERFORMANCE_MEASUREMENT
#include <qdatetime.h>

// This is the central timer used for performance measurement. It is
// set to zero and started when the kdvi_multipage is
// constructed. This object is statically defined in
// kdvi_multipage.cpp.
extern QTime performanceTimer;

// A flag that is set to true once the first page of the document was
// successfully drawn. This object is statically defined in
// kdvi_multipage.cpp.
// 0 = initial value
// 1 = first page was drawn
// 2 = last page was drawn
extern int performanceFlag;
#endif
