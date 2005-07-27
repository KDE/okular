/***************************************************************************
 *   Copyright (C) 1997-2005 the KGhostView authors. See file GV_AUTHORS.  *
 *                                                                         *
 *   Many portions of this file are based on kghostview's kpswidget code   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <kdialogbase.h>

class QLabel;
class QTextEdit;
class KURLLabel;

class GVLogWindow : public KDialogBase
{
    Q_OBJECT

public:
    GVLogWindow( const QString& caption,
               QWidget* parent = 0, const char* name = 0 );

public slots:
    void append( const QString& message );
    void append( char* buf, int num );
    void clear(); 
    void setLabel( const QString&, bool showConfigureGSLink );

private slots:
    void emitConfigureGS();

signals:
    void configureGS();

private:
    QLabel*      m_errorIndication;
    QTextEdit*   m_logView;
    KURLLabel*   m_configureGS;
};

#endif

// vim:sw=4:sts=4:ts=8:sta:tw=78:noet
