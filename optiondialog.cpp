/*
 *   kdvi: KDE dvi viewer
 *   This file: Copyright (C) 2000 Espen Sand, espen@kde.org
 *   Based on ealier dialog code 
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

// Add header files alphabetically

#include <kapp.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kinstance.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include "fontpool.h"
#include "optiondialog.h"
#include <config.h>

OptionDialog::OptionDialog( QWidget *parent, const char *name, bool modal )
  :KDialogBase( Tabbed, i18n("Preferences"), Help|Ok|Apply|Cancel, Ok,
		parent, name, modal )
{
  _instance = new KInstance("kdvi");
  setHelp("opts", "kdvi");

  makeFontPage();
  makeRenderingPage();

  KConfig *config = _instance->config();
  config->setGroup("kdvi");

  // Font page
  // Important! The default values here must be the same as in kdvi_multipage.cpp
  for(int i=0; i<NumberOfMFModes; i++)
    mFont.metafontMode->insertItem(QString("%1 dpi / %2").arg(MFResolutions[i]).arg(MFModenames[i]));
  mFont.metafontMode->setCurrentItem( config->readNumEntry( "MetafontMode" , DefaultMFMode ));
  mFont.fontPathCheck->setChecked( config->readBoolEntry( "MakePK", true ) );

  // Rendering page
  mRender.showSpecialCheck->setChecked( config->readNumEntry( "ShowPS", 1 ) );
  mRender.showHyperLinksCheck->setChecked(config->readNumEntry("ShowHyperLinks", 1)); 
}


void OptionDialog::show()
{
  KConfig *config = _instance->config();
  config->reparseConfiguration();
  config->setGroup("kdvi");
  
  mFont.metafontMode->setCurrentItem( config->readNumEntry( "MetafontMode" , DefaultMFMode ));
  mFont.fontPathCheck->setChecked( config->readBoolEntry( "MakePK", true ) );

  // Rendering page
  mRender.showSpecialCheck->setChecked( config->readNumEntry( "ShowPS", 1 ) );
  mRender.showHyperLinksCheck->setChecked(config->readNumEntry("ShowHyperLinks", 1)); 

  if( isVisible() == false ) 
    showPage(0);
  KDialogBase::show();
}

void OptionDialog::slotOk()
{
  slotApply();
  accept();
}

void OptionDialog::slotApply()
{
  KConfig *config = _instance->config();
  config->setGroup("kdvi");

  config->writeEntry( "MetafontMode", mFont.metafontMode->currentItem() );
  config->writeEntry( "MakePK", mFont.fontPathCheck->isChecked() );
  config->writeEntry( "ShowPS", mRender.showSpecialCheck->isChecked() );
  config->writeEntry( "ShowHyperLinks", mRender.showHyperLinksCheck->isChecked() );

  config->sync();

  emit preferencesChanged();
}


void OptionDialog::makeFontPage()
{
  QFrame *page = addPage( i18n("Fonts") );
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
  mFont.pageIndex = pageIndex(page);

  QGridLayout *glay = new QGridLayout(topLayout, 8, 2 );

  QLabel *label = new QLabel( i18n("Metafont mode:"), page );
  mFont.metafontMode = new KComboBox( page );
  QToolTip::add( mFont.metafontMode, i18n("LaserJet 4 is usually a good choice.") );
  QWhatsThis::add( mFont.metafontMode, i18n("Chooses the type of bitmap fonts used for the display. As a general rule, the higher the dpi value, the better quality of the output. On the other hand, large dpi fonts use more resources and make KDVI slower. \nIf you are low on hard disk space, or have a slow machine, you may want to choose the same setting that is also used by dvips. That way you avoid to generate several bitmap versions of the same font.") );

  glay->addWidget( label, 0, 0 );
  glay->addWidget( mFont.metafontMode, 0, 1 );

  glay->addRowSpacing( 2, spacingHint()*2 );

  mFont.fontPathCheck = new QCheckBox( i18n("Generate missing fonts"), page );
  QToolTip::add( mFont.fontPathCheck, i18n("If in doubt, switch on!") );
  QWhatsThis::add( mFont.fontPathCheck, i18n("Allows KDVI to use MetaFont to produce bitmap fonts. Unless you have a very specific reason, you probably want to switch this on.") );
  glay->addMultiCellWidget( mFont.fontPathCheck, 3, 3, 0, 1 );

  topLayout->addStretch(1);
}


void OptionDialog::makeRenderingPage()
{
  QFrame *page = addPage( i18n("Rendering") );
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
  mRender.pageIndex = pageIndex(page);

  mRender.showSpecialCheck = new QCheckBox( i18n("Show PostScript specials"), page );
  QToolTip::add( mRender.showSpecialCheck, i18n("If in doubt, switch on!") );
  QWhatsThis::add( mRender.showSpecialCheck, i18n("Some DVI files contain PostScript graphics. If switched on, KDVI will use the ghostview PostScript interpreter to display these. You probably want to switch this option on, unless you have a DVI-file whose PostScript part is broken, or too large for your machine.") );
  mRender.showHyperLinksCheck =  new QCheckBox( i18n("Show Hyperlinks"), page );  
  QToolTip::add( mRender.showHyperLinksCheck, i18n("If in doubt, switch on!") );
  QWhatsThis::add( mRender.showHyperLinksCheck, i18n("For your convenience, some DVI files contain hyperlinks which are corss-references or point to external documents. You probably want to switch this option on, unless you are annoyed by the blue underlines which KDVI uses to mark the hyperlinks.") );
  topLayout->addWidget( mRender.showSpecialCheck );
  topLayout->addWidget( mRender.showHyperLinksCheck );

  topLayout->addStretch(1);
}


#include "optiondialog.moc"
