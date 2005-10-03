// -*- C++ -*-
// infodialog.h
//
// (C) 2001 Stefan Kebekus
// Distributed under the GPL

#ifndef INFO_KDVI_H
#define INFO_KDVI_H

#include <kdialogbase.h>

#include <qstring.h>

class dvifile;
class fontPool;
class QTextView;
class QWidget;


class infoDialog : public KDialogBase
{
    Q_OBJECT

public:
    infoDialog( QWidget* parent = 0 );

    /** This method is used to set the data coming from the DVI
        file. Note that 0 is a permissible argument, that just means:
        "no file loaded" */
    void setDVIData(dvifile *dviFile);

    QTextView* TextLabel1;
    QTextView* TextLabel2;
    QTextView* TextLabel3;

public slots:
    /** This slot is called when Output from the MetaFont programm
        is received via the fontpool/kpsewhich */
    void       outputReceiver(const QString&);

    /** This slot is called whenever anything in the fontpool has
        changed. If the infoDialog is shown, the dialog could then
        query the fontpool for more information. */
    void       setFontInfo(fontPool *fp);

    /** Calling this slot clears the text view and stores the
	headline. The next time output is received via the
	outputReceiver, the headline is displayed in bold on top of
	the text view. */
    void       clear(const QString&);

protected:
 bool     MFOutputReceived;
 QString  headline;
 QString  pool;
};

#endif // INFO_KDVI_H
