// optiondialog.cpp
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

// Add header files alphabetically

#include <kdebug.h>
#include <klocale.h>
#include <qvbox.h>

#include "optiondialog.h"
#include "optionDialogFontsWidget.h"
#include "optionDialogSpecialWidget.h"


OptionDialog::OptionDialog( QWidget *parent, const char *name, bool modal )
  :KDialogBase( Tabbed, i18n("Preferences"), Help|Ok|Apply|Cancel, Ok,
		parent, name, modal )
{
  setHelp("opts", "kdvi");

  QWidget *page;
  page = new optionDialogFontsWidget(addVBoxPage(i18n("TeX Fonts")));
  connect(this, SIGNAL(apply()), page, SLOT(apply()));
  connect(this, SIGNAL(okClicked()), page, SLOT(apply()));
  
  page = new optionDialogSpecialWidget(addVBoxPage(i18n("DVI Specials")));
  connect(this, SIGNAL(apply()), page, SLOT(apply()));
  connect(this, SIGNAL(okClicked()), page, SLOT(apply()));
}

void OptionDialog::slotOk(void)
{
  // Beware! I could not connect preferencesChanged to the signal
  // okClicked because slots are called in arbitrary order.
  emit okClicked();
  emit preferencesChanged();
  accept();
}

void OptionDialog::slotApply(void)
{
  // Beware! I could not connect preferencesChanged to the signal
  // apply because slots are called in arbitrary order.
  emit okClicked();
  emit preferencesChanged();
}

#include "optiondialog.moc"
