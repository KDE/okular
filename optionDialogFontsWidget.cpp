// optionDiologWidget.cpp
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

// Add header files alphabetically

#include <kdebug.h>

#include <kconfig.h>
#include <kcombobox.h>
#include <kinstance.h>
#include <klocale.h>
#include <qcheckbox.h>

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
  config->writeEntry( "MetafontMode", metafontMode->currentItem() );
  config->writeEntry( "MakePK", fontGenerationCheckBox->isChecked() );
  config->writeEntry( "enlarge_for_readability", fontEnlargementCheckBox->isChecked() );
  config->sync();
}


#include "optionDialogFontsWidget.moc"
