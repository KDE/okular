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

#include "konq_progressproxy.h"

#include <kbrowser.h>

#include <kio_job.h>

KonqProgressProxy::KonqProgressProxy( BrowserView *view, KIOJob *job )
: QObject( job )
{
  
  connect( job, SIGNAL( sigTotalSize( int, unsigned long ) ), this, SLOT( slotTotalSize( int, unsigned long ) ) );
  connect( job, SIGNAL( sigProcessedSize( int, unsigned long ) ), this, SLOT( slotProcessedSize( int, unsigned long ) ) );
  connect( job, SIGNAL( sigSpeed( int, unsigned long ) ), this, SLOT( slotSpeed( int, unsigned long ) ) );

  connect( this, SIGNAL( loadingProgress( int ) ), view, SIGNAL( loadingProgress( int ) ) );
  connect( this, SIGNAL( speedProgress( int ) ), view, SIGNAL( speedProgress( int ) ) );

  m_ulTotalDocumentSize = 0;
}

void KonqProgressProxy::slotTotalSize( int, unsigned long size )
{
  m_ulTotalDocumentSize = size;
}

void KonqProgressProxy::slotProcessedSize( int, unsigned long size )
{
  if ( m_ulTotalDocumentSize > (unsigned long)0 )
    emit loadingProgress( size * 100 / m_ulTotalDocumentSize );
}

void KonqProgressProxy::slotSpeed( int, unsigned long bytesPerSecond )
{
  emit speedProgress( (long int)bytesPerSecond );
}

#include "konq_progressproxy.moc"
