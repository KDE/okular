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

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QSpinBox;

#include <kdialogbase.h>

class OptionDialog : public KDialogBase
{
  Q_OBJECT


  struct FontItems
  {
    int pageIndex;
    QLineEdit   *resolutionEdit;
    QLineEdit   *metafontEdit;
    QCheckBox   *fontPathCheck;
    QLabel      *fontPathLabel;
    QLineEdit   *fontPathEdit;
  };
    
  struct RenderItems
  {
    int pageIndex;
    QCheckBox *showSpecialCheck;
    QCheckBox *antialiasCheck;
    QLineEdit *gammaEdit;
  };

  public:
    OptionDialog( QWidget *parent=0, const char *name=0, bool modal=true);
    virtual void show();
    static bool paperSizes( const char *p, float &w, float &h );

  protected slots:
    virtual void slotOk();
    virtual void slotApply();

  private:
    void setup();
    void makeFontPage();
    void makeRenderingPage();

  private slots:
    void fontPathCheckChanged( bool state );

  signals:
    void preferencesChanged();

  private:
    FontItems   mFont;
    RenderItems mRender;
};


#endif
