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

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qspinbox.h>

#include <kapp.h>
#include <kconfig.h>
#include <klocale.h>
#include <kglobal.h>

#include "optiondialog.h"


OptionDialog::OptionDialog( QWidget *parent, const char *name, bool modal )
  :KDialogBase( Tabbed, i18n("Preferences"), Help|Ok|Apply|Cancel, Ok,
		parent, name, modal )
{
  makeFontPage();
  makeRenderingPage();
}


void OptionDialog::show()
{
  if( isVisible() == false )
  {
    setup();
    showPage(0);
  }
  KDialogBase::show();
}


void OptionDialog::slotOk()
{
  slotApply();
  accept();
}

void OptionDialog::slotApply()
{
  KConfig *config = KGlobal::config();
  config->setGroup("kdvi");

  config->writeEntry( "BaseResolution", mFont.resolutionEdit->text() );
  config->writeEntry( "MetafontMode", mFont.metafontEdit->text() );
  config->writeEntry( "MakePK", mFont.fontPathCheck->isChecked() );
  config->writeEntry( "FontPath", mFont.fontPathEdit->text() );

  config->writeEntry( "ShowPS", mRender.showSpecialCheck->isChecked() );
  config->writeEntry( "PS Anti Alias", mRender.antialiasCheck->isChecked() );

  config->sync();

  emit preferencesChanged();
}


void OptionDialog::setup()
{
  KConfig *config = KGlobal::config();
  config->setGroup("kdvi");

  // Font page
  mFont.resolutionEdit->setText( config->readEntry( "BaseResolution" ) );
  mFont.metafontEdit->setText( config->readEntry( "MetafontMode" ) );
  mFont.fontPathCheck->setChecked( config->readNumEntry( "MakePK" ) );
  mFont.fontPathEdit->setText( config->readEntry( "FontPath" ) );
  fontPathCheckChanged( mFont.fontPathCheck->isChecked() );

  // Rendering page
  mRender.showSpecialCheck->setChecked( config->readNumEntry( "ShowPS" ) );
  mRender.antialiasCheck->setChecked(config->readNumEntry("PS Anti Alias", 1)); 
}



void OptionDialog::makeFontPage()
{
  QFrame *page = addPage( i18n("Fonts") );
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
  mFont.pageIndex = pageIndex(page);

  QGridLayout *glay = new QGridLayout(topLayout, 8, 2 );
  QLabel *label = new QLabel( i18n("Resolution [dpi]:"), page );
  mFont.resolutionEdit = new QLineEdit( page );
  glay->addWidget( label, 0, 0 );
  glay->addWidget( mFont.resolutionEdit, 0, 1 );

  label = new QLabel( i18n("Metafont mode:"), page );
  mFont.metafontEdit = new QLineEdit( page );
  glay->addWidget( label, 1, 0 );
  glay->addWidget( mFont.metafontEdit, 1, 1 );

  glay->addRowSpacing( 2, spacingHint()*2 );

  mFont.fontPathCheck = new QCheckBox( i18n("Generate missing fonts"), page );
  connect( mFont.fontPathCheck, SIGNAL(toggled(bool)), 
	   this, SLOT(fontPathCheckChanged(bool)) );
  glay->addMultiCellWidget( mFont.fontPathCheck, 3, 3, 0, 1 );

  mFont.fontPathLabel = new QLabel( i18n("PK font path:"), page );
  mFont.fontPathEdit = new QLineEdit( page );
  glay->addWidget( mFont.fontPathLabel, 4, 0 );
  glay->addWidget( mFont.fontPathEdit, 4, 1 );

  topLayout->addStretch(1);
}


void OptionDialog::makeRenderingPage()
{
  QFrame *page = addPage( i18n("Rendering") );
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
  mRender.pageIndex = pageIndex(page);

  mRender.showSpecialCheck = 
    new QCheckBox( i18n("Show PostScript specials"), page );
  mRender.antialiasCheck = 
    new QCheckBox( i18n("Antialiased PostScript"), page );  
  topLayout->addWidget( mRender.showSpecialCheck );
  topLayout->addWidget( mRender.antialiasCheck );

  topLayout->addStretch(1);
}


void OptionDialog::fontPathCheckChanged( bool state )
{
  mFont.fontPathLabel->setEnabled( state );
  mFont.fontPathEdit->setEnabled( state );
}


