
#ifndef _KPDF_INTERPETER_H_
#define _KPDF_INTERPETER_H_

#include <qpair.h>
#include <qpaintdevice.h>
// include "dscparse_adapter.h"
typedef QPair<unsigned long , unsigned long > PagePosition;
typedef enum MessageType{Input, Output, Error};

namespace DPIMod
{
        const float X = QPaintDevice::x11AppDpiX()/72.0;
        const float Y = QPaintDevice::x11AppDpiY()/72.0;
}

#endif
