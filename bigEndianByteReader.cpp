// bigEndianByteReader.cpp
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#include <config.h>

#include <kdebug.h>

#include "bigEndianByteReader.h"
#include "dvi.h"

//#define DEBUG_ENDIANREADER

Q_UINT8 bigEndianByteReader::readUINT8()
{
  // This check saveguards us against segmentation fault. It is also
  // necessary for virtual fonts, which do not end whith EOP.
  if (command_pointer >= end_pointer) {
#ifdef DEBUG_ENDIANREADER
    kdError(4300) << "bigEndianByteReader::readUINT8() tried to read past end of data chunk" << endl;
    kdError(4300) << "end_pointer     = " << end_pointer << endl;
    kdError(4300) << "command_pointer = " << command_pointer << endl;
#endif
    return EOP;
  }

  return *(command_pointer++);
}

Q_UINT16 bigEndianByteReader::readUINT16()
{
  // This check saveguards us against segmentation fault. It is also
  // necessary for virtual fonts, which do not end whith EOP.
  if (command_pointer >= end_pointer)
    return EOP;

  Q_UINT16 a;
  a = *(command_pointer++);
  a = (a << 8) | *(command_pointer++);
  return a;
}

Q_UINT32 bigEndianByteReader::readUINT32()
{
  // This check saveguards us against segmentation fault. It is also
  // necessary for virtual fonts, which do not end whith EOP.
  if (command_pointer >= end_pointer)
    return EOP;

  Q_UINT32 a;
  a = *(command_pointer++);
  a = (a << 8) | *(command_pointer++);
  a = (a << 8) | *(command_pointer++);
  a = (a << 8) | *(command_pointer++);
  return a;
}

void bigEndianByteReader::writeUINT32(Q_UINT32 a)
{
  // This check saveguards us against segmentation fault. It is also
  // necessary for virtual fonts, which do not end whith EOP.
  if (command_pointer >= end_pointer)
    return;

  command_pointer[3] = (Q_UINT8)(a & 0xFF);
  a = a >> 8;
  command_pointer[2] = (Q_UINT8)(a & 0xFF);
  a = a >> 8;
  command_pointer[1] = (Q_UINT8)(a & 0xFF);
  a = a >> 8;
  command_pointer[0] = (Q_UINT8)(a & 0xFF);

  command_pointer += 4;
  return;
}

Q_UINT32 bigEndianByteReader::readUINT(Q_UINT8 size)
{
  // This check saveguards us against segmentation fault. It is also
  // necessary for virtual fonts, which do not end whith EOP.
  if (command_pointer >= end_pointer)
    return EOP;

  Q_UINT32 a = 0;
  while (size > 0) { 
    a = (a << 8) + *(command_pointer++);
    size--;
  }
  return a;
}

Q_INT32 bigEndianByteReader::readINT(Q_UINT8 length)
{
  // This check saveguards us against segmentation fault. It is also
  // necessary for virtual fonts, which do not end whith EOP.
  if (command_pointer >= end_pointer)
    return EOP;

  Q_INT32 a = *(command_pointer++);

  if (a & 0x80)
    a -= 0x100;

  while ((--length) > 0)
    a = (a << 8) | *(command_pointer++);

  return a;
}
