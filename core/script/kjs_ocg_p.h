/***************************************************************************
 *   Copyright (C) 2019 by João Netto <joaonetto901@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_SCRIPT_KJS_OCG_P_H
#define OKULAR_SCRIPT_KJS_OCG_P_H

class KJSContext;
class KJSObject;
class QAbstractItemModel;

namespace Okular
{
class JSOCG
{
public:
    static void initType(KJSContext *ctx);
    static KJSObject object(KJSContext *ctx);
    static KJSObject wrapOCGObject(KJSContext *ctx, QAbstractItemModel *model, const int i, const int j);
    static void clearCachedFields();
};

}

#endif
