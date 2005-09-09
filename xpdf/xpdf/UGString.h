//========================================================================
//
// UGString.h
//
// Unicode string
//
// Copyright 2005 Albert Astals Cid <aacid@kde.org>
//
//========================================================================

#ifndef UGSTRING_H
#define UGSTRING_H

#include "CharTypes.h"

class GString;

class UGString
{
public:
  // Create an unicode string
  UGString(Unicode *u, int l);

  // Create a unicode string from <str>.
  UGString(GString &str);

  // Copy the unicode string
  UGString(const UGString &str);

  // Create a unicode string from <str>.
  UGString(const char *str);

  // Destructor.
  ~UGString();

  // Get length.
  int getLength() const { return length; }

  // Compare two strings:  -1:<  0:=  +1:>
  int cmp(UGString *str) const;

  // get the unicode
  Unicode *unicode() const { return s; }

  // get the const char*
  const char *getCString() const;

private:
  void initChar(GString &str);

  int length;
  Unicode *s;
};

#endif
