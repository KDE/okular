// optionDiologWidget.cpp
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

// Add header files alphabetically

#include <kconfig.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kinstance.h>
#include <klocale.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include "../config.h"
#include "fontpool.h"
#include "optionDialogFontsWidget.h"


// Constructs a optionDialogWidget_base which is a child of 'parent', with
// the name 'name' and widget flags set to 'f'.
optionDialogFontsWidget::optionDialogFontsWidget( QWidget* parent,  const char* name, WFlags fl )
    : optionDialogFontsWidget_base( parent,  name, fl )
{
  instance = 0;
  config = 0;
  instance = new KInstance("kdvi");
  config = instance->config();
  
  // Important! The default values here must be the same as in kdvi_multipage.cpp
  for(int i=0; i<NumberOfMFModes; i++)
    metafontMode->insertItem(QString("%1 dpi / %2").arg(MFResolutions[i]).arg(MFModenames[i]));
  
  config->setGroup("kdvi");
  metafontMode->setCurrentItem( config->readNumEntry( "MetafontMode" , DefaultMFMode ));
  
#ifdef HAVE_FREETYPE
  usePFBCheckBox->setChecked( config->readBoolEntry( "UsePFB", true ) );
  useFontHintingCheckBox->setChecked( config->readBoolEntry( "UsePFBFontHints", true ) );
  useFontHintingCheckBox->setEnabled(usePFBCheckBox->isChecked());
#else
  usePFBCheckBox->setChecked(false);
  usePFBCheckBox->setEnabled(false);
  useFontHintingCheckBox->setEnabled(false);
  useFontHintingCheckBox->setChecked(false);
  QToolTip::add(PFB_ButtonGroup, i18n("This version of KDVI does not support type 1 fonts."));
  QWhatsThis::add(PFB_ButtonGroup, i18n("KDVI needs the FreeType library to access type 1 fonts. This library "
					"was not present when KDVI was compiled. If you want to use type 1 "
					"fonts, you must either install the FreeType library and recompile KDVI "
					"yourself, or find a precompiled software package for your operating "
					"system."));
#endif

  fontGenerationCheckBox->setChecked( config->readBoolEntry( "MakePK", true ) );
  fontEnlargementCheckBox->setChecked( config->readBoolEntry( "enlarge_for_readability", true ) );
}

optionDialogFontsWidget::~optionDialogFontsWidget()
{
  if (instance != 0L)
    delete instance; // that will also delete the config
}

void optionDialogFontsWidget::apply(void)
{
  config->setGroup("kdvi");
#ifdef HAVE_FREETYPE
  config->writeEntry( "UsePFB", usePFBCheckBox->isChecked() );
  config->writeEntry( "UsePFBFontHints", useFontHintingCheckBox->isChecked() );
#endif
  config->writeEntry( "MetafontMode", metafontMode->currentItem() );
  config->writeEntry( "MakePK", fontGenerationCheckBox->isChecked() );
  config->writeEntry( "enlarge_for_readability", fontEnlargementCheckBox->isChecked() );
  config->sync();
}


#include "optionDialogFontsWidget.moc"
