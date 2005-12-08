// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// optionDialogSpecialWidget.cpp
//
// Part of KDVI - A DVI previewer for the KDE desktop environment
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#include <config.h>

#include "optionDialogSpecialWidget.h"
#include "prefs.h"

#include <kcombobox.h>
#include <kdebug.h>
#include <klineedit.h>
#include <klocale.h>
#include <ktoolinvocation.h>
#include <kurllabel.h>

#include <QCheckBox>


// Constructs a optionDialogWidget_base which is a child of 'parent', with
// the name 'name' and widget flags set to 'f'.
optionDialogSpecialWidget::optionDialogSpecialWidget( QWidget* parent,  const char* name, Qt::WFlags fl )
    : optionDialogSpecialWidget_base( parent,  name, fl )
{
  // Set up the list of known and supported editors
  editorNameString        += i18n("User-Defined Editor");
  editorCommandString     += "";
  editorDescriptionString += i18n("Enter the command line below.");

  editorNameString        += "Emacs / emacsclient";
  editorCommandString     += "emacsclient --no-wait +%l %f || emacs +%l %f";
  editorDescriptionString += i18n("Click 'Help' to learn how to set up Emacs.");

  editorNameString        += "Kate";
  editorCommandString     += "kate --use --line %l %f";
  editorDescriptionString += i18n("Kate perfectly supports inverse search.");

  editorNameString        += "Kile";
  editorCommandString     += "kile %f --line %l";
  editorDescriptionString += i18n("Kile works very well");

  editorNameString        += "NEdit";
  editorCommandString     += "ncl -noask -line %l %f || nc -noask -line %l %f";
  editorDescriptionString += i18n("NEdit perfectly supports inverse search.");

  editorNameString        += "VIM - Vi IMproved / GUI";
  editorCommandString     += "gvim --servername KDVI --remote-silent +%l %f";
  editorDescriptionString += i18n("VIM version 6.0 or greater works just fine.");

  editorNameString        += "XEmacs / gnuclient";
  editorCommandString     += "gnuclient -q +%l %f || xemacs  +%l %f";
  editorDescriptionString += i18n("Click 'Help' to learn how to set up XEmacs.");

  for(int i=0; i<editorNameString.count(); i++)
    editorChoice->insertItem(editorNameString[i]);
  // Set the proper editor on the "Rendering-Page", try to recognize
  // the editor command from the config-file. If the editor command is
  // not recognized, switch to "User defined editor". That way, kdvi
  // stays compatible even if the EditorCommands[] change between
  // different versions of kdvi.
  QString currentEditorCommand = Prefs::editorCommand();
  int i;
  for(i = editorCommandString.count()-1; i>0; i--)
    if (editorCommandString[i] == currentEditorCommand)
      break;
  if (i == 0)
    usersEditorCommand = currentEditorCommand;
  slotComboBox(i);

  connect(urll, SIGNAL(leftClickedURL(const QString&)), this, SLOT(slotExtraHelpButton(const QString&)));
  connect(editorChoice, SIGNAL( activated( int ) ), this, SLOT( slotComboBox( int ) ) );

  // Editor description strings (and their translations) vary in
  // size. Find the longest description string available to make sure
  // that the page is always large enough.
  int maximumWidth = 0;
  for ( QStringList::Iterator it = editorDescriptionString.begin(); it != editorDescriptionString.end(); ++it ) {
    int width = editorDescription->fontMetrics().width(*it);
    if (width > maximumWidth)
      maximumWidth = width;
  }
  editorDescription->setMinimumWidth(maximumWidth+10);

  connect(kcfg_EditorCommand, SIGNAL( textChanged (const QString &) ), this, SLOT( slotUserDefdEditorCommand( const QString & ) ) );
}

optionDialogSpecialWidget::~optionDialogSpecialWidget()
{
}

void optionDialogSpecialWidget::slotUserDefdEditorCommand( const QString &text )
{
  if (isUserDefdEditor == true)
    EditorCommand = usersEditorCommand = text;
}


void optionDialogSpecialWidget::slotComboBox(int item)
{
  if (item != editorChoice->currentItem())
    editorChoice->setCurrentItem(item);

  editorDescription->setText(editorDescriptionString[item]);

  if (item != 0) {
    isUserDefdEditor = false;
    kcfg_EditorCommand->setText(editorCommandString[item]);
    kcfg_EditorCommand->setReadOnly(true);
    EditorCommand = editorCommandString[item];
  } else {
    kcfg_EditorCommand->setText(usersEditorCommand);
    kcfg_EditorCommand->setReadOnly(false);
    EditorCommand = usersEditorCommand;
    isUserDefdEditor = true;
  }
}

void optionDialogSpecialWidget::slotExtraHelpButton( const QString & )
{
  KToolInvocation::invokeHelp("inv-search", "kdvi");
}

void optionDialogSpecialWidget::apply()
{
  Prefs::setEditorCommand(EditorCommand);
}


#include "optionDialogSpecialWidget.moc"
