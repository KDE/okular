// optiondialog.h
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#ifndef OPTIONDIALOG
#define OPTIONDIALOG

#include <kdialogbase.h>

class OptionDialog : public KDialogBase
{
  Q_OBJECT

  public:
    OptionDialog( QWidget *parent=0, const char *name=0, bool modal=true);

  protected slots:
    void slotOk(void);
    void slotApply(void);

  signals:
    void preferencesChanged();
};


#endif
