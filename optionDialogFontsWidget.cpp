// optionDiologWidget.cpp
//
// Part of KDVI - A DVI previewer for the KDE desktop environment
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#include <config.h>

#include "optionDialogFontsWidget.h"

#ifndef HAVE_FREETYPE
# include <klocale.h>

# include <qcheckbox.h>
# include <qtooltip.h>
# include <qwhatsthis.h>
#endif // HAVE_FREETYPE


// Constructs a optionDialogWidget_base which is a child of 'parent', with
// the name 'name' and widget flags set to 'f'.
optionDialogFontsWidget::optionDialogFontsWidget(QWidget* parent,
                                                 const char* name,
                                                 WFlags fl)
  : optionDialogFontsWidget_base(parent,  name, fl)
{
#ifndef HAVE_FREETYPE
  kcfg_UseFontHints->setChecked(false);
  kcfg_UseFontHints->setEnabled(false);
  kcfg_UseFontHints->setEnabled(false);
  kcfg_UseFontHints->setChecked(false);
  QToolTip::add(kcfg_UseFontHints,
                i18n("This version of KDVI does not support type 1 fonts."));
  QWhatsThis::add(kcfg_UseFontHints,
                i18n("KDVI needs the FreeType library to access type 1 fonts. "
                     "This library was not present when KDVI was compiled. "
                     "If you want to use type 1 fonts, you must either "
                     "install the FreeType library and recompile KDVI "
                     "yourself, or find a precompiled software package "
                     "for your operating system."));
#endif
}

optionDialogFontsWidget::~optionDialogFontsWidget()
{}

#include "optionDialogFontsWidget.moc"
