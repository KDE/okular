// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
/* This file is part of KDVI (C) 2001 by Stefan Kebekus (kebekus@kde.org)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License
   as published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.
*/

/**
 * Byte reading routines which read big endian numbers from memory and
 * convert them to native integers.
 *
 * @author Stefan Kebekus (kebekus@kde.org)
 *
 **/

#ifndef _bigEndianByteReader_H
#define _bigEndianByteReader_H

#include <QGlobalStatic>

class bigEndianByteReader
{
public:
    /** Set this pointer to the location where the number resides which
        you want to read. */
    quint8 *command_pointer;

    /** This pointer marks the end of the memory area where bytes can be
        read. It should point to the first byte which CANNOT be
        read. The idea is to have a safety net which protects us against
        SEGFAULTs. This is also used in virtual fonts, where the macro
        does not have an EOP command at the end of the macro. */
    quint8 *end_pointer;

    /** If command_pointer >= end_pointer, this method return EOP (=140)
        and exists. Otherwise, the method returns the unsigned byte
        and increases the command_pointer by one. */
    quint8 readUINT8();

    /** Similar to the method above, only that the method reads a big
        endian 2-byte word and increases the pointer by two. */
    quint16 readUINT16();

    /** Similar to the method above, only that the method reads a big
        endian 4-byte word and increases the pointer by four. */
    quint32 readUINT32();

    void writeUINT32(quint32 a);

    /** Similar to the method above, only that the method reads a big
        endian number of length size, where 1 <= size <= 4. Note that
        the value 3 is allowed (and is actually used in DVI files)!!!  */
    quint32 readUINT(quint8 size);

    /** Similar to the method above, only that the method reads a SIGNED
        number */
    qint32 readINT(quint8);
};

#endif // ifndef _bigEndianByteReader_H
