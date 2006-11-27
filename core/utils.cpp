/***************************************************************************
 *   Copyright (C) 2006 by Luigi Toscano <luigi.toscano@tiscali.it>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "utils.h"

#include <QtCore/QRect>

#ifdef Q_WS_X11
#include <QX11Info>
#endif

#ifdef Q_WS_MAC
#include <ApplicationServices/ApplicationServices.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#endif



using namespace Okular;

QRect Utils::rotateRect( const QRect & source, int width, int height, int orientation )
{
    QRect ret;

    // adapt the coordinates of the boxes to the rotation
    switch ( orientation )
    {
        case 1:
            ret = QRect( width - source.y() - source.height(), source.x(),
                         source.height(), source.width() );
            break;
        case 2:
            ret = QRect( width - source.x() - source.width(), height - source.y() - source.height(),
                         source.width(), source.height() );
            break;
        case 3:
            ret = QRect( source.y(), height - source.x() - source.width(),
                         source.height(), source.width() );
            break;
        case 0:  // no modifications
        default: // other cases
            ret = source;
    }

    return ret;
}

#if defined(Q_WS_X11)

double Utils::dpiX()
{
    return QX11Info::appDpiX();
}

double Utils::dpiY()
{
    return QX11Info::appDpiY();
}

#elif defined(Q_WS_MAC)
    /*
     * Code copied from http://developer.apple.com/qa/qa2001/qa1217.html
     */
    //    Handy utility function for retrieving an int from a CFDictionaryRef
    static int GetIntFromDictionaryForKey( CFDictionaryRef desc, CFStringRef key )
    {
        CFNumberRef value;
        int num = 0;
        if ( (value = (CFNumberRef)CFDictionaryGetValue(desc, key)) == NULL || CFGetTypeID(value) != CFNumberGetTypeID())
            return 0;
        CFNumberGetValue(value, kCFNumberIntType, &num);
        return num;
    }

    static CGDisplayErr GetDisplayDPI( CFDictionaryRef displayModeDict, CGDirectDisplayID displayID,
                                       double *horizontalDPI, double *verticalDPI )
    {
        CGDisplayErr err = kCGErrorFailure;
        io_connect_t displayPort;
        CFDictionaryRef displayDict;

        //    Grab a connection to IOKit for the requested display
        displayPort = CGDisplayIOServicePort( displayID );
        if ( displayPort != MACH_PORT_NULL )
        {
            //    Find out what IOKit knows about this display
            displayDict = IODisplayCreateInfoDictionary(displayPort, 0);
            if ( displayDict != NULL )
            {
                const double mmPerInch = 25.4;
                double horizontalSizeInInches =
                    (double)GetIntFromDictionaryForKey(displayDict,
                                                       CFSTR(kDisplayHorizontalImageSize)) / mmPerInch;
                double verticalSizeInInches =
                    (double)GetIntFromDictionaryForKey(displayDict,
                                                       CFSTR(kDisplayVerticalImageSize)) / mmPerInch;

                //    Make sure to release the dictionary we got from IOKit
                CFRelease(displayDict);

                // Now we can calculate the actual DPI
                // with information from the displayModeDict
                *horizontalDPI =
                    (double)GetIntFromDictionaryForKey( displayModeDict, kCGDisplayWidth )
                    / horizontalSizeInInches;
                *verticalDPI = (double)GetIntFromDictionaryForKey( displayModeDict,
                        kCGDisplayHeight ) / verticalSizeInInches;
                err = CGDisplayNoErr;
            }
        }
        return err;
    }

double Utils::dpiX()
{
    double x,y;
    CGDisplayErr err = GetDisplayDPI( CGDisplayCurrentMode(kCGDirectMainDisplay),
                                      kCGDirectMainDisplay,
                                      &x, &y );

    return err == CGDisplayNoErr ? x : 72.0;
}

double Utils::dpiY()
{
    double x,y;
    CGDisplayErr err = GetDisplayDPI( CGDisplayCurrentMode(kCGDirectMainDisplay),
                                      kCGDirectMainDisplay,
                                      &x, &y );

    return err == CGDisplayNoErr ? y : 72.0;
}
#else
#error "Not yet contributed"
#endif
