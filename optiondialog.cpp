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

#include "optiondialog.h"



static const char *paperNames[] = 
{ 
  "US",
  "US landscape",
  "Legal",
  "foolscap",
  
  // ISO `A' formats, Portrait
  "A1",
  "A2",
  "A3",
  "A4",
  "A5",
  "A6",
  "A7",

  // ISO `A' formats, Landscape
  "A1 landscape",
  "A2 landscape",
  "A3 landscape",
  "A4 landscape",
  "A5 landscape",
  "A6 landscape",
  "A7 landscape",

  // ISO `B' formats, Portrait
  "B1",
  "B2",
  "B3",
  "B4",
  "B5",
  "B6",
  "B7",

  // ISO `B' formats, Landscape
  "B1 landscape",
  "B2 landscape",
  "B3 landscape",
  "B4 landscape",
  "B5 landscape",
  "B6 landscape",
  "B7 landscape",

  // ISO `C' formats, Portrait
  "C1",
  "C2",
  "C3",
  "C4",
  "C5",
  "C6",
  "C7",

  // ISO `C' formats, Landscape
  "C1 landscape",
  "C2 landscape",
  "C3 landscape",
  "C4 landscape",
  "C5 landscape",
  "C6 landscape",
  "C7 landscape",

  0
};


static const char *papers[] = 
{ 
  "us",		"8.5x11",
  "usr",	"11x8.5",
  "legal",	"8.5x14",
  "foolscap",	"13.5x17.0",	/* ??? */
  
  // ISO `A' formats, Portrait 
  "a1",		"59.4x84.0cm",
  "a2",		"42.0x59.4cm",
  "a3",		"29.7x42.0cm",
  "a4",		"21.0x29.7cm",
  "a5",		"14.85x21.0cm",
  "a6",		"10.5x14.85cm",
  "a7",		"7.42x10.5cm",
  
  // ISO `A' formats, Landscape
  "a1r",	"84.0x59.4cm",
  "a2r",	"59.4x42.0cm",
  "a3r",	"42.0x29.7cm",
  "a4r",	"29.7x21.0cm",
  "a5r",	"21.0x14.85cm",
  "a6r",	"14.85x10.5cm",
  "a7r",	"10.5x7.42cm",
  
  // ISO `B' formats, Portrait
  "b1",		"70.6x100.0cm",
  "b2",		"50.0x70.6cm",
  "b3",		"35.3x50.0cm",
  "b4",		"25.0x35.3cm",
  "b5",		"17.6x25.0cm",
  "b6",		"13.5x17.6cm",
  "b7",		"8.8x13.5cm",
  
  // ISO `B' formats, Landscape
  "b1r",	"100.0x70.6cm",
  "b2r",	"70.6x50.0cm",
  "b3r",	"50.0x35.3cm",
  "b4r",	"35.3x25.0cm",
  "b5r",	"25.0x17.6cm",
  "b6r",	"17.6x13.5cm",
  "b7r",	"13.5x8.8cm",
  
  // ISO `C' formats, Portrait
  "c1",		"64.8x91.6cm",
  "c2",		"45.8x64.8cm",
  "c3",		"32.4x45.8cm",
  "c4",		"22.9x32.4cm",
  "c5",		"16.2x22.9cm",
  "c6",		"11.46x16.2cm",
  "c7",		"8.1x11.46cm",
  
  // ISO `C' formats, Landscape
  "c1r",        "91.6x64.8cm",
  "c2r",	"64.8x45.8cm",
  "c3r",	"45.8x32.4cm",
  "c4r",	"32.4x22.9cm",
  "c5r",	"22.9x16.2cm",
  "c6r",	"16.2x11.46cm",
  "c7r",	"11.46x8.1cm",

  0
};





OptionDialog::OptionDialog( QWidget *parent, const char *name, bool modal )
  :KDialogBase( Tabbed, i18n("Preferences"), Help|Ok|Apply|Cancel, Ok,
		parent, name, modal )
{
  makeFontPage();
  makeRenderingPage();
  makePaperPage();
  makeMiscPage();
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
  KConfig &config = *kapp->config();
  config.setGroup("kdvi");

  config.writeEntry( "BaseResolution", mFont.resolutionEdit->text() );
  config.writeEntry( "MetafontMode", mFont.metafontEdit->text() );
  config.writeEntry( "MakePK", mFont.fontPathCheck->isChecked() );
  config.writeEntry( "FontPath", mFont.fontPathEdit->text() );
  config.writeEntry( "SmallShrink", mFont.shrinkSmallEdit->text() );
  config.writeEntry( "LargeShrink", mFont.shrinkLargeEdit->text() );

  config.writeEntry( "ShowPS", mRender.showSpecialCheck->isChecked() );
  config.writeEntry( "PS Anti Alias", mRender.antialiasCheck->isChecked() );
  config.writeEntry( "Gamma", mRender.gammaEdit->text() );

  QString paperText = mPaper.paperCombo->currentText();
  config.writeEntry( "Paper", paperText.simplifyWhiteSpace() );

  config.setGroup("RecentFiles");
  config.writeEntry( "MaxCount", mMisc.recentSpin->value() );

  emit preferencesChanged();
}


void OptionDialog::setup()
{
  KConfig &config = *kapp->config();
  config.setGroup("kdvi");

  // Font page
  mFont.resolutionEdit->setText( config.readEntry( "BaseResolution" ) );
  mFont.metafontEdit->setText( config.readEntry( "MetafontMode" ) );
  mFont.fontPathCheck->setChecked( config.readNumEntry( "MakePK" ) );
  mFont.fontPathEdit->setText( config.readEntry( "FontPath" ) );
  mFont.shrinkSmallEdit->setText( config.readEntry( "SmallShrink" ) );
  mFont.shrinkLargeEdit->setText( config.readEntry( "LargeShrink" ) );
  fontPathCheckChanged( mFont.fontPathCheck->isChecked() );

  // Rendering page
  mRender.showSpecialCheck->setChecked( config.readNumEntry( "ShowPS" ) );
  mRender.antialiasCheck->setChecked(config.readNumEntry("PS Anti Alias", 1)); 
  mRender.gammaEdit->setText( config.readEntry( "Gamma" ) );

  // Paper page
  QString paperText = config.readEntry( "Paper" );
  if( mPaper.paperCombo->currentText() != paperText )
  {
    bool match = false;
    for( int i=0; i<mPaper.paperCombo->count() && papers[i*2] != 0; i++ )
    {
      if( paperText == mPaper.paperCombo->text(i) || paperText == papers[i*2] )
      {
	match = true;
	mPaper.paperCombo->setCurrentItem(i);
	break;
      }
    }
    if( match == false )
    {
      mPaper.paperCombo->insertItem( paperText, 0 );
      mPaper.paperCombo->setCurrentItem(0);
    }
  }

  // Misc page
  config.setGroup("RecentFiles");
  mMisc.recentSpin->setValue( config.readEntry( "MaxCount", "5" ).toInt() );
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

  glay->addRowSpacing( 5, spacingHint()*2 );
  
  label = new QLabel( i18n("Shrink factor for small text:"), page );
  mFont.shrinkSmallEdit = new QLineEdit( page );
  glay->addWidget( label, 6, 0 );
  glay->addWidget( mFont.shrinkSmallEdit, 6, 1 );

  label = new QLabel( i18n("Shrink factor for large text:"), page );
  mFont.shrinkLargeEdit = new QLineEdit( page );
  glay->addWidget( label, 7, 0 );
  glay->addWidget( mFont.shrinkLargeEdit, 7, 1 );

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

  QHBoxLayout *hlay = new QHBoxLayout( topLayout );
  QLabel *label = new QLabel( i18n("Gamma value:"), page );
  mRender.gammaEdit = new QLineEdit( page );
  hlay->addWidget( label );
  hlay->addWidget( mRender.gammaEdit, 1 );

  topLayout->addStretch(1);
}


void OptionDialog::makePaperPage()
{
  QFrame *page = addPage( i18n("Paper") );
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
  mPaper.pageIndex = pageIndex(page);

  QLabel *label = new QLabel( i18n("Select paper size:"), page );
  topLayout->addWidget( label );
  
  mPaper.paperCombo = new QComboBox( false, page );
  topLayout->addWidget( mPaper.paperCombo );
  mPaper.paperCombo->insertStrList ( paperNames );

  topLayout->addStretch(1);
}


void OptionDialog::makeMiscPage()
{
  QFrame *page = addPage( i18n("Miscellaneous") );
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
  mMisc.pageIndex = pageIndex(page);

  QHBoxLayout *hlay = new QHBoxLayout( topLayout );
  QLabel *label = new QLabel( i18n("Number of recent files:"), page );
  hlay->addWidget( label );

  mMisc.recentSpin = new QSpinBox( 0, 100, 1, page );
  hlay->addWidget( mMisc.recentSpin );

  topLayout->addStretch(1);
}


void OptionDialog::fontPathCheckChanged( bool state )
{
  mFont.fontPathLabel->setEnabled( state );
  mFont.fontPathEdit->setEnabled( state );
}


static bool OptionDialog::paperSizes( const char *p, float &w, float &h )
{
  QString s(p);
  s = s.simplifyWhiteSpace();
  
  for( uint i=0; paperNames[i] != 0; i++ )
  {
    if( s == paperNames[i] )
    {
      s = papers[ 2*i + 1 ];
      int cm = s.findRev( "cm" );
      w = s.toFloat();
      int ind = s.find( 'x' );
      if ( ind<0 )
	return false;
      s = s.mid( ind + 1, 9999 );
      h = s.toFloat();
      if ( cm >= 0 )
	w /= 2.54, h /= 2.54;
      return true;
    }
  }
  return false;
}

