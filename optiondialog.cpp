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

#include <config.h>
#include <kapplication.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kinstance.h>
#include <klineedit.h>
#include <klocale.h>
#include <kurllabel.h>
#include <qcheckbox.h>
#include <qfontmetrics.h>
#include <qgrid.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qwhatsthis.h>

#include "fontpool.h"
#include "optiondialog.h"



OptionDialog::OptionDialog( QWidget *parent, const char *name, bool modal )
  :KDialogBase( Tabbed, i18n("Preferences"), Help|Ok|Apply|Cancel, Ok,
		parent, name, modal )
{
  _instance = new KInstance("kdvi");
  setHelp("opts", "kdvi");

  // Set up the list of known and supported editors
  EditorNames        += i18n("User-Defined Editor");
  EditorCommands     += "";
  EditorDescriptions += i18n("Enter the command line below.");
  
  EditorNames        += "Emacs / emacsclient";
  EditorCommands     += "emacsclient --no-wait +%l %f || emacs +%l %f";
  EditorDescriptions += i18n("Click 'Help' to learn how to set up Emacs.");
  
  EditorNames        += "Kate";
  EditorCommands     += "kate %f";
  EditorDescriptions += i18n("Kate does not jump to line");
  
  EditorNames        += "NEdit";
  EditorCommands     += "ncl -noask -line %l %f || nc -noask -line %l %f";
  EditorDescriptions += i18n("NEdit perfectly supports inverse search.");
  
  EditorNames        += "VIM - Vi IMproved / GUI";
  EditorCommands     += "gvim --servername kdvi --remote +%l %f";
  EditorDescriptions += i18n("VIM version 6.0 or greater works just fine.");

  EditorNames        += "XEmacs / gnuclient";
  EditorCommands     += "gnuclient -q +%l %f || xemacs  +%l %f";
  EditorDescriptions += i18n("Click 'Help' to learn how to set up XEmacs.");

  
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
  mRender.showSpecialCheck->setChecked(config->readBoolEntry("ShowPS", true));
  mRender.showHyperLinksCheck->setChecked(config->readBoolEntry("ShowHyperLinks", true)); 
  for(unsigned int i=0; i<EditorNames.count(); i++)
    mRender.editorChoice->insertItem(EditorNames[i]);
  // Set the proper editor on the "Rendering-Page", try to recognize
  // the editor command from the config-file. If the editor command is
  // not recognized, switch to "User defined editor". That way, kdvi
  // stays compatible even if the EditorCommands[] change between
  // different versions of kdvi.
  QString currentEditorCommand = config->readEntry( "EditorCommand", "" );
  int i;
  for(i = EditorCommands.count()-1; i>0; i--)
    if (EditorCommands[i] == currentEditorCommand)
      break;
  if (i == 0)
    usersEditorCommand = currentEditorCommand;
  slotComboBox(i);
}


void OptionDialog::show()
{
  KConfig *config = _instance->config();
  config->reparseConfiguration();
  config->setGroup("kdvi");
  
  mFont.metafontMode->setCurrentItem( config->readNumEntry( "MetafontMode" , DefaultMFMode ));
  mFont.fontPathCheck->setChecked( config->readBoolEntry( "MakePK", true ) );

  // Rendering page
  mRender.showSpecialCheck->setChecked( config->readBoolEntry( "ShowPS", true));
  mRender.showHyperLinksCheck->setChecked(config->readBoolEntry("ShowHyperLinks", true)); 

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

void OptionDialog::slotExtraHelpButton( const QString &anchor)
{
  kapp->invokeHelp( anchor, "kdvi" );
}

void OptionDialog::makeFontPage()
{
  QGrid *page = addGridPage( 2, QGrid::Horizontal, i18n("Fonts") );

  new QLabel( i18n("Metafont mode:"), page );
  mFont.metafontMode = new KComboBox( page );
  QToolTip::add( mFont.metafontMode, i18n("LaserJet 4 is usually a good choice.") );
  QWhatsThis::add( mFont.metafontMode, i18n("Chooses the type of bitmap fonts used for the display. As a general rule, the higher the dpi value, the better quality of the output. On the other hand, large dpi fonts use more resources and make KDVI slower. \nIf you are low on hard disk space, or have a slow machine, you may want to choose the same setting that is also used by dvips. That way you avoid to generate several bitmap versions of the same font.") );

  mFont.fontPathCheck = new QCheckBox( i18n("Generate missing fonts"), page );
  QToolTip::add( mFont.fontPathCheck, i18n("If in doubt, switch on!") );
  QWhatsThis::add( mFont.fontPathCheck, i18n("Allows KDVI to use MetaFont to produce bitmap fonts. Unless you have a very specific reason, you probably want to switch this on.") );
}

void OptionDialog::makeRenderingPage()
{
  QVBox *page = addVBoxPage( i18n("DVI Specials") );

  mRender.showSpecialCheck = new QCheckBox( i18n("Show PostScript specials"), page );
  QToolTip::add( mRender.showSpecialCheck, i18n("If in doubt, switch on!") );
  QWhatsThis::add( mRender.showSpecialCheck, i18n("Some DVI files contain PostScript graphics. If switched on, KDVI will use the ghostview PostScript interpreter to display these. You probably want to switch this option on, unless you have a DVI-file whose PostScript part is broken, or too large for your machine.") );

  mRender.showHyperLinksCheck =  new QCheckBox( i18n("Show hyperlinks"), page );
  QToolTip::add( mRender.showHyperLinksCheck, i18n("If in doubt, switch on!") );
  QWhatsThis::add( mRender.showHyperLinksCheck, i18n("For your convenience, some DVI files contain hyperlinks which are cross-references or point to external documents. You probably want to switch this option on, unless you don't want the blue underlines which KDVI uses to mark the hyperlinks.") );

  QGroupBox *editorBox = new QGroupBox( 2, Horizontal, i18n("Editor for Inverse Search"), page ); 

  new QLabel( "", editorBox );
  KURLLabel *urll = new KURLLabel("inv-search", i18n("What is 'inverse search'?"), editorBox, "inverse search help");
  urll->setAlignment(Qt::AlignRight);
  connect(urll, SIGNAL(leftClickedURL(const QString&)),  SLOT(slotExtraHelpButton(const QString&)));

  new QLabel( i18n("Editor:"), editorBox );
  mRender.editorChoice =  new KComboBox( editorBox );
  connect(mRender.editorChoice, SIGNAL( activated( int ) ), this, SLOT( slotComboBox( int ) ) );
  QToolTip::add( mRender.editorChoice, i18n("Choose an editor which is used in inverse search.") );
  QWhatsThis::add( mRender.editorChoice, i18n("Some DVI-files contain 'inverse search' information. If such a DVI-file is loaded, you can click with the right mouse into KDVI, an editor opens, loads the TeX-file and jumps to the proper position. You can select you favourite editor here. If in doubt, 'nedit' is usually a good choice.\nCheck the KDVI manual to see how to prepare DVI-files which support the inverse search.") );

  // Editor description strings (and their translations) vary in
  // size. Find the longest description string available to make sure
  // that the page is always large enough.
  new QLabel( i18n("Description:"), editorBox );
  mRender.editorDescription = new QLabel( editorBox );
  int maximumWidth = 0;
  for ( QStringList::Iterator it = EditorDescriptions.begin(); it != EditorDescriptions.end(); ++it ) {
    int width = mRender.editorDescription->fontMetrics().width(*it);
    if (width > maximumWidth) 
      maximumWidth = width;
  }
  mRender.editorDescription->setMinimumWidth(maximumWidth+10);
  QToolTip::add( mRender.editorDescription, i18n("Explains about the editor's capabilities in conjunction with inverse search.") );
  QWhatsThis::add( mRender.editorDescription, i18n("Not all editors are well-suited for inverse search. For instance, many editors have no command like 'If the file is not yet loaded, load it. Otherwise, bring the window with the file to the front'. If you are using an editor like this, clicking into the DVI file will always open a new editor, even if the TeX-file is already open. Likewise, many editors have no command line argument that would allow KDVI to specify the exact line which you wish to edit.\nIf you feel that KDVI's support for a certain editor is not well-done, please write to kebekus@kde.org.") );

  new QLabel( i18n("Shell command:"), editorBox );
  mRender.editorCallingCommand =  new KLineEdit( editorBox );
  mRender.editorCallingCommand->setReadOnly(true);
  connect(mRender.editorCallingCommand, SIGNAL( textChanged (const QString &) ), this, SLOT( slotUserDefdEditorCommand( const QString & ) ) );
  QToolTip::add( mRender.editorCallingCommand, i18n("Shell-command line used to start the editor.") );
  QWhatsThis::add( mRender.editorCallingCommand, i18n("If you are using inverse search, KDVI uses this command line to start the editor. The field '%f' is replaced with the filename, and '%l' is replaced with the line number.") );
}


#include "optiondialog.moc"
