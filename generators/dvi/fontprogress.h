// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
//
// fontprogress.h
//
// (C) 2001-2004 Stefan Kebekus
// Distributed under the GPL

#ifndef FONT_GENERATION_H
#define FONT_GENERATION_H

#include <kdialog.h>

#include <QPointer>

class QProgressBar;
class QLabel;
class QProcess;


/**
 *  A dialog to give feedback to the user when kpsewhich is generating fonts.
 *
 * This class implements a dialog which pops up, shows a progress bar
 * and displays the MetaFont output. It contains three slots,
 * outputReceiver, setTotalSteps and hideDialog which can be connected
 * with the appropriate signals emitted by the fontpool class.
 *
 * @author Stefan Kebekus   <kebekus@kde.org>
 *
 *
 **/
class fontProgressDialog : public KDialog
{
    Q_OBJECT

public:
    fontProgressDialog(const QString& helpIndex, const QString& label, const QString& abortTip, const QString& whatsThis, const QString& ttip,
                       QWidget* parent = 0, bool progressbar=true );
    ~fontProgressDialog();

    /** The number of steps already done is increased, the text received
        here is analyzed and presented to the user. */
    void increaseNumSteps(const QString& explanation);

    /** Used to initialize the progress bar. If the argument @c proc is
        non-zero, the associated process will be killed when the "abort"
        button is pressed. The pointer is stored internally inside a
        QPointer, so it is safe to delete the real QProcess instance
        at any time. */
    void setTotalSteps(int, QProcess* proc=0);

    QLabel* TextLabel2;

private slots:
    /** Calling this slot does nothing than to kill the process that is
        pointed to be procIO, if procIO is not zero.*/
  void killProcess();

private:
   QLabel* TextLabel1;
   QProgressBar* ProgressBar1;
   int progress;
   QPointer<QProcess> process;
};

#endif // FONT_GENERATION_H
