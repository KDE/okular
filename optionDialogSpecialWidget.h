// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
// optionDialogSpecialWidget.h
//
// Part of KDVI - A DVI previewer for the KDE desktop environment
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#ifndef OPTIONDIALOGSPECIALWIDGET_H
#define OPTIONDIALOGSPECIALWIDGET_H

#include "optionDialogSpecialWidget_base.h"


class optionDialogSpecialWidget : public optionDialogSpecialWidget_base
{
  Q_OBJECT

 public:
  optionDialogSpecialWidget( QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = 0 );
  ~optionDialogSpecialWidget();

 public slots:
  void apply();
  void slotComboBox(int item);
  void slotUserDefdEditorCommand( const QString &text );
  void slotExtraHelpButton( const QString &anchor);

 private:
  QStringList editorNameString, editorCommandString, editorDescriptionString;
  QString     EditorCommand;
  bool        isUserDefdEditor;
  QString     usersEditorCommand;
};

#endif // OPTIONDIALOGSPECIALWIDGET_H
