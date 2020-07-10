/***************************************************************************
 *   Copyright (C) 2018 by Intevation GmbH <intevation@intevation.de>      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_SCRIPT_KJS_EVENT_P_H
#define OKULAR_SCRIPT_KJS_EVENT_P_H

class KJSContext;
class KJSObject;

namespace Okular
{
class Event;

class JSEvent
{
public:
    static void initType(KJSContext *ctx);
    static KJSObject wrapEvent(KJSContext *ctx, Event *event);
    static void clearCachedFields();
};

}

#endif
