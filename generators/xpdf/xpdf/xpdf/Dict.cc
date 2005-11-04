//========================================================================
//
// Dict.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stddef.h>
#include <string.h>
#include "gmem.h"
#include "Object.h"
#include "XRef.h"
#include "Dict.h"
#include "UGString.h"

//------------------------------------------------------------------------
// Dict
//------------------------------------------------------------------------

Dict::Dict(XRef *xrefA) {
  xref = xrefA;
  entries = NULL;
  size = length = 0;
  ref = 1;
}

Dict::~Dict() {
  int i;

  for (i = 0; i < length; ++i) {
    delete entries[i].key;
    entries[i].val.free();
  }
  gfree(entries);
}

void Dict::add(const UGString &key, Object *val) {
  if (length == size) {
    if (length == 0) {
      size = 8;
    } else {
      size *= 2;
    }
    entries = (DictEntry *)greallocn(entries, size, sizeof(DictEntry));
  }
  entries[length].key = new UGString(key);
  entries[length].val = *val;
  ++length;
}

inline DictEntry *Dict::find(const UGString &key) {
  int i;

  for (i = 0; i < length; ++i) {
    if (!key.cmp(entries[i].key))
      return &entries[i];
  }
  return NULL;
}

GBool Dict::is(const char *type) {
  DictEntry *e;

  return (e = find("Type")) && e->val.isName(type);
}

Object *Dict::lookup(const UGString &key, Object *obj) {
  DictEntry *e;

  return (e = find(key)) ? e->val.fetch(xref, obj) : obj->initNull();
}

Object *Dict::lookupNF(const UGString &key, Object *obj) {
  DictEntry *e;

  return (e = find(key)) ? e->val.copy(obj) : obj->initNull();
}

UGString *Dict::getKey(int i) {
  return entries[i].key;
}

Object *Dict::getVal(int i, Object *obj) {
  return entries[i].val.fetch(xref, obj);
}

Object *Dict::getValNF(int i, Object *obj) {
  return entries[i].val.copy(obj);
}
