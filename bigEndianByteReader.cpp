// Copyright notice missing.

#include "bigEndianByteReader.h"
#include "dvi.h"

Q_UINT8 bigEndianByteReader::readUINT8(void)
{
  // This check saveguards us against segmentation fault. It is also
  // necessary for virtual fonts, which do not end whith EOP.
  if (command_pointer >= end_pointer)
    return EOP;

  return *(command_pointer++);
}

Q_UINT16 bigEndianByteReader::readUINT16(void)
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

Q_UINT32 bigEndianByteReader::readUINT32(void)
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
