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
#ifndef _OPTION_DIALOG_H_
#define _OPTION_DIALOG_H_

class KComboBox;
class KInstance;
class KLineEdit;
class QCheckBox;
class QLabel;
class QLineEdit;
class QSpinBox;

#include <kdialogbase.h>

class OptionDialog : public KDialogBase
{
  Q_OBJECT


  struct FontItems
  {
    int         pageIndex;
    KComboBox  *metafontMode;
    QCheckBox  *fontPathCheck;
  };
    
  struct RenderItems
  {
    int         pageIndex;
    QCheckBox  *showSpecialCheck;
    QCheckBox  *showHyperLinksCheck;
    KComboBox  *editorChoice;
    QLabel     *editorDescription;
    KLineEdit  *editorCallingCommand;
  };

  public:
    OptionDialog( QWidget *parent=0, const char *name=0, bool modal=true);
    virtual void show();
    QString     EditorCommand;

  protected slots:
    void slotOk();
    void slotApply();
    void slotComboBox(int item);
    void slotUserDefdEditorCommand( const QString & );
    void slotExtraHelpButton( const QString &);

  private:
    void makeFontPage();
    void makeRenderingPage();

  signals:
    void preferencesChanged();

  private:
    bool        isUserDefdEditor;
    QString     usersEditorCommand;
    FontItems   mFont;
    RenderItems mRender;

    QStringList EditorNames;
    QStringList EditorCommands;
    QStringList EditorDescriptions;

    KInstance   *_instance;
};


#endif
