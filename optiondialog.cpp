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
#include <kglobal.h>
#include <kinstance.h>
#include <klineedit.h>
#include <klocale.h>
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

  // Set up the list of known and supported editors
  EditorNames        += "User-defined editor";
  EditorCommands     += "";
  EditorDescriptions += i18n("Enter the command line below.");

  EditorNames        += "Kate";
  EditorCommands     += "kate %f";
  EditorDescriptions += i18n("Kate does not jump to line");

  EditorNames        += "NEdit";
  EditorCommands     += "ncl -noask -line %l %f";
  EditorDescriptions += i18n("NEdit perfectly supports inverse search.");

  EditorNames        += "XEmacs";
  EditorCommands     += "xemacs %f";
  EditorDescriptions += i18n("XEmacs always opens a new window, does not jump to line");

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
  for(unsigned int i=0; i<EditorNames.count(); i++)
    mRender.editorChoice->insertItem(EditorNames[i]);
  mRender.editorCallingCommand->setText(EditorCommands[0]);
  // Missing: Read Entry from config file.
  int item = 1;
  slotComboBox(item);
  //    mFont.metafontMode->setCurrentItem( config->readNumEntry( "MetafontMode" , DefaultMFMode ));
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
  config->writeEntry( "EditorCommand", EditorCommand );

  config->sync();

  emit preferencesChanged();
}

void OptionDialog::slotUserDefdEditorCommand( const QString &text )
{
  if (isUserDefdEditor == true)
    EditorCommand = usersEditorCommand = text;
}

void OptionDialog::slotComboBox(int item)
{
  if (item != mRender.editorChoice->currentItem())
    mRender.editorChoice->setCurrentItem(item);

  mRender.editorDescription->setText(EditorDescriptions[item]);

  if (item != 0) {
    isUserDefdEditor = false;
    mRender.editorCallingCommand->setText(EditorCommands[item]);
    mRender.editorCallingCommand->setReadOnly(true);
    EditorCommand = EditorCommands[item];
  } else {
    mRender.editorCallingCommand->setText(usersEditorCommand);
    mRender.editorCallingCommand->setReadOnly(false);
    EditorCommand = usersEditorCommand;
    isUserDefdEditor = true;
  }
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

  QGridLayout *glay = new QGridLayout(topLayout, 8, 2 );

  mRender.showSpecialCheck = new QCheckBox( i18n("Show PostScript specials"), page );
  QToolTip::add( mRender.showSpecialCheck, i18n("If in doubt, switch on!") );
  QWhatsThis::add( mRender.showSpecialCheck, i18n("Some DVI files contain PostScript graphics. If switched on, KDVI will use the ghostview PostScript interpreter to display these. You probably want to switch this option on, unless you have a DVI-file whose PostScript part is broken, or too large for your machine.") );
  glay->addWidget( mRender.showSpecialCheck, 0, 0 );

  mRender.showHyperLinksCheck =  new QCheckBox( i18n("Show Hyperlinks"), page );  
  QToolTip::add( mRender.showHyperLinksCheck, i18n("If in doubt, switch on!") );
  QWhatsThis::add( mRender.showHyperLinksCheck, i18n("For your convenience, some DVI files contain hyperlinks which are corss-references or point to external documents. You probably want to switch this option on, unless you are annoyed by the blue underlines which KDVI uses to mark the hyperlinks.") );
  glay->addWidget( mRender.showHyperLinksCheck, 1, 0 );

  glay->addRowSpacing( 2, spacingHint()*2 );

  QLabel *label = new QLabel( i18n("Editor for inverse search:"), page );
  glay->addWidget( label, 2, 0 );
  mRender.editorChoice =  new KComboBox( page );
  connect(mRender.editorChoice, SIGNAL( activated( int ) ), this, SLOT( slotComboBox( int ) ) );
  QToolTip::add( mRender.editorChoice, i18n("Choose an editor which is used in inverse search.") );
  QWhatsThis::add( mRender.editorChoice, i18n("Some DVI-files contain 'inverse search' information. If such a DVI-file is loaded, you can click with the right mouse into KDVI, an editor opens, loads the TeX-file and jumps to the proper position. You can select you favourite editor here. If in doubt, 'nedit' is usually a good choice.\nCheck the KDVI manual to see how to prepare DVI-files which support the inverse search.") );
  glay->addWidget( mRender.editorChoice, 2, 1 );

  label = new QLabel( i18n("Editor description:"), page );
  glay->addWidget( label, 3, 0 );
  // Find the longest description string available, to make sure that
  // the page is always large enough. Of course, we are making a
  // mistake here, since we use variable-width fonts. Let's hope that
  // this is not going to be trouble.
  QString longest;
  unsigned int size = 0;
  kdError() << "Test" << endl;
  for ( QStringList::Iterator it = EditorDescriptions.begin(); it != EditorDescriptions.end(); ++it ) {
    kdError() << *it << endl;
    if ((*it).length() > size) {
      longest = *it;
      size = longest.length();
    }
  }
  kdError() << longest << endl;
  mRender.editorDescription = new QLabel( longest, page );
  QToolTip::add( mRender.editorDescription, i18n("Explains about the editor's capabilities in conjunction with inverse search.") );
  QWhatsThis::add( mRender.editorDescription, i18n("Not all editors are well-suited for inverse search. For instance, many editors have no command like 'If the file is not yet loaded, load it. Otherwise, bring the window with the file to the front'. If you are using an editor like this, clicking into the DVI file will always open a new editor, even if the TeX-file is already open. Likewise, many editors have no command line argument that would allow KDVI to specify the exact line which you wish to edit.\nIf you feel that KDVI's support for a certain editor is not well-done, please write to kebekus@kde.org.") );
  glay->addWidget( mRender.editorDescription, 3, 1 );

  label = new QLabel( i18n("Command to start the editor:"), page );
  glay->addWidget( label, 4, 0 );
  mRender.editorCallingCommand =  new KLineEdit( page );
  mRender.editorCallingCommand->setReadOnly(true);
  connect(mRender.editorCallingCommand, SIGNAL( textChanged (const QString &) ), this, SLOT( slotUserDefdEditorCommand( const QString & ) ) );
  QToolTip::add( mRender.editorCallingCommand, i18n("Shell-command line used start the editor.") );
  QWhatsThis::add( mRender.editorCallingCommand, i18n("If you are using inverse search, KDVI uses this command line to start the editor. The field '%f' is replaced with the filename, and '%l' is replaced with the line number.") );
  glay->addWidget( mRender.editorCallingCommand, 4, 1 );
  topLayout->addStretch(1);
}


#include "optiondialog.moc"
