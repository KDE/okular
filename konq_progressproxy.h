/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
*/ 

#ifndef __konq_progressproxy_h__
#define __konq_progressproxy_h__

#include <qobject.h>

class BrowserView;
class KIOJob;

class KonqProgressProxy : public QObject
{
  Q_OBJECT
public:
  KonqProgressProxy( BrowserView *view, KIOJob *job );

protected slots:
  void slotTotalSize( int, unsigned long size );
  void slotProcessedSize( int, unsigned long size );
  void slotSpeed( int, unsigned long bytesPerSecond );

signals:
  void loadingProgress( int percent );
  void speedProgress( int bytesPerSecond );

private:
  unsigned long m_ulTotalDocumentSize;
};

#endif
