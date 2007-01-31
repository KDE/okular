/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_GLOBAL_H
#define OKULAR_GLOBAL_H

#include <QtCore/QGlobalStatic>

namespace Okular {

/**
 * Describes the DRM capabilities.
 */
enum Permission
{
    AllowModify = 1,  ///< Allows to modify the document
    AllowCopy = 2,    ///< Allows to copy the document
    AllowPrint = 4,   ///< Allows to print the document
    AllowNotes = 8    ///< Allows to add annotations to the document
};
Q_DECLARE_FLAGS( Permissions, Permission )

/**
 * Describes the direction of searching.
 */
enum SearchDirection
{
    FromTop,        ///< Searching from top of the page, next result is to be found, there was no earlier search result.
    FromBottom,     ///< Searching from bottom of the page, next result is to be found, there was no earlier search result.
    NextResult,     ///< Searching for the next result on the page, earlier result should be located so we search from the last result not from the beginning of the page.
    PreviousResult  ///< Searching for the previous result on the page, earlier result should be located so we search from the last result not from the beginning of the page.
};

/**
 * A rotation.
 */
enum Rotation
{
    Rotation0 = 0,    ///< Not rotated.
    Rotation90 = 1,   ///< Rotated 90 degrees clockwise.
    Rotation180 = 2,  ///< Rotated 180 degrees clockwise.
    Rotation270 = 3   ///< Rotated 2700 degrees clockwise.
};

/**
 * Describes the type of generation of objects
 */
enum GenerationType
{
    Synchronous,      ///< Will create the object in a synchronous way
    Asynchronous      ///< Will create the object in an asynchronous way
};

}

#endif
