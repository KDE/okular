// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
// optionDialogFontsWidget.h
//
// Part of KDVI - A DVI previewer for the KDE desktop environment
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#ifndef OPTIONDIALOGFONTSWIDGET_H
#define OPTIONDIALOGFONTSWIDGET_H

#include "optionDialogFontsWidget_base.h"


class optionDialogFontsWidget : public optionDialogFontsWidget_base
{
  Q_OBJECT

 public:
  optionDialogFontsWidget( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
  ~optionDialogFontsWidget();
};

#endif // OPTIONDIALOGFONTSWIDGET_H
