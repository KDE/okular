/***************************************************************************
 *   Copyright (C) 2006 by Luigi Toscano <luigi.toscano@tiscali.it>        *
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "utils.h"
#include "utils_p.h"

#include <QtCore/QRect>
#include <QApplication>
#include <QDesktopWidget>
#include <QImage>
#include <QIODevice>

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

double Utils::realDpiX()
{
    const QDesktopWidget* w = QApplication::desktop();
    return (double(w->width()) * 25.4) / double(w->widthMM());
}

double Utils::realDpiY()
{
    const QDesktopWidget* w = QApplication::desktop();
    return (double(w->height()) * 25.4) / double(w->heightMM());
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

double Utils::realDpiX()
{
    return dpiX();
}

double Utils::realDpiY()
{
    return dpiY();
}
#else

double Utils::dpiX()
{
    return QDesktopWidget().physicalDpiX();
}

double Utils::dpiY()
{
    return QDesktopWidget().physicalDpiY();
}

double Utils::realDpiX()
{
    return dpiX();
}

double Utils::realDpiY()
{
    return dpiY();
}
#endif

inline static bool isWhite( QRgb argb ) {
    return ( argb & 0xFFFFFF ) == 0xFFFFFF; // ignore alpha
}

NormalizedRect Utils::imageBoundingBox( const QImage * image )
{
    if ( !image )
        return NormalizedRect();

    int width = image->width();
    int height = image->height();
    int left, top, bottom, right, x, y;

#ifdef BBOX_DEBUG
    QTime time;
    time.start();
#endif

    // Scan pixels for top non-white
    for ( top = 0; top < height; ++top )
        for ( x = 0; x < width; ++x )
            if ( !isWhite( image->pixel( x, top ) ) )
                goto got_top;
    return NormalizedRect( 0, 0, 0, 0 ); // the image is blank
got_top:
    left = right = x;

    // Scan pixels for bottom non-white
    for ( bottom = height-1; bottom >= top; --bottom )
        for ( x = width-1; x >= 0; --x )
            if ( !isWhite( image->pixel( x, bottom ) ) )
                goto got_bottom;
    Q_ASSERT( 0 ); // image changed?!
got_bottom:
    if ( x < left )
        left = x;
    if ( x > right )
        right = x;

    // Scan for leftmost and rightmost (we already found some bounds on these):
    for ( y = top; y <= bottom && ( left > 0 || right < width-1 ); ++y )
    {
        for ( x = 0; x < left; ++x )
            if ( !isWhite( image->pixel( x, y ) ) )
                left = x;
        for ( x = width-1; x > right+1; --x )
            if ( !isWhite( image->pixel( x, y ) ) )
                right = x;
    }

    NormalizedRect bbox( QRect( left, top, ( right - left + 1), ( bottom - top + 1 ) ),
                         image->width(), image->height() );

#ifdef BBOX_DEBUG
    kDebug() << "Computed bounding box" << bbox << "in" << time.elapsed() << "ms";
#endif

    return bbox;
}

void Okular::copyQIODevice( QIODevice *from, QIODevice *to )
{
    QByteArray buffer( 65536, '\0' );
    qint64 read = 0;
    qint64 written = 0;
    while ( ( read = from->read( buffer.data(), buffer.size() ) ) > 0 )
    {
        written = to->write( buffer.constData(), read );
        if ( read != written )
            break;
    }
}
