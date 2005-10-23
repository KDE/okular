// optionDiologWidget.cpp
//
// Part of KDVI - A DVI previewer for the KDE desktop environment
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#include <config.h>

#include "optionDialogFontsWidget.h"
#include "fontpool.h"

#include <kcombobox.h>
#include <kdebug.h>
#include <klocale.h>

#include <Q3ButtonGroup>
#include <QCheckBox>
#include <QToolTip>


// Constructs a optionDialogWidget_base which is a child of 'parent', with
// the name 'name' and widget flags set to 'f'.
optionDialogFontsWidget::optionDialogFontsWidget( QWidget* parent,  const char* name, Qt::WFlags fl )
    : optionDialogFontsWidget_base( parent,  name, fl )
{
#ifndef HAVE_FREETYPE
  kcfg_UseType1Fonts->setChecked(false);
  kcfg_UseType1Fonts->setEnabled(false);
  kcfg_UseFontHints->setEnabled(false);
  kcfg_UseFontHints->setChecked(false);
  QToolTip::add(PFB_ButtonGroup, i18n("This version of KDVI does not support type 1 fonts."));
  PFB_ButtonGroup->setWhatsThis( i18n("KDVI needs the FreeType library to access type 1 fonts. This library "
                                      "was not present when KDVI was compiled. If you want to use type 1 "
                                      "fonts, you must either install the FreeType library and recompile KDVI "
                                      "yourself, or find a precompiled software package for your operating "
                                      "system."));
#endif
}

optionDialogFontsWidget::~optionDialogFontsWidget()
{
}

#include "optionDialogFontsWidget.moc"
