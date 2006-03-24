/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include "interpreter.h"
#include <kdialogbase.h>
#include <klistviewsearchline.h>
#include <qvbox.h>
#include <qtimer.h>

class QTextEdit;
class KUrlLabel;
class GSLogWindow : public QVBox
{
    Q_OBJECT

public:
    GSLogWindow( const QString& caption,
               QWidget* parent = 0, const char* name = 0 );

public slots:
    void append( GSInterpreterLib::MessageType t, const char* buf, int num );
    void appendBuffered() { m_lastInt=0; m_clearTimer.stop(); append(m_buffer.first,m_buffer.second); };
    void append( GSInterpreterLib::MessageType t, const QString &text);
    void clear();
    bool event( QEvent * event );

private:
    KListView*   m_msgList;
    KListViewSearchLine* m_searchLine;
    QPair <GSInterpreterLib::MessageType, QString> m_buffer;
    int m_tCol;
    int m_lastInt;
    QTimer m_clearTimer;
};

#endif

// vim:sw=4:sts=4:ts=8:sta:tw=78:noet
