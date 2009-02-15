/***************************************************************************
 *   Copyright (C) 2009 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "animatedwidget.h"

#include <qevent.h>
#include <qpainter.h>

#include <kiconloader.h>

AnimatedWidget::AnimatedWidget( const QString& iconName, QWidget *parent )
    : QWidget( parent ), m_icon( iconName ), m_frames( 0 ), m_currentFrame( 0 ),
    m_size( 0 )
{
    setAutoFillBackground( false );

    hide();
}

AnimatedWidget::~AnimatedWidget()
{
}

void AnimatedWidget::start()
{
    if ( m_timer.isActive() )
       return;

    if ( !m_frames )
    {
        load();
        if ( !m_frames )
            return;
    }

    m_timer.start( 1000 / m_frames, this );
    show();
}

void AnimatedWidget::stop()
{
    m_timer.stop();
    m_currentFrame = 0;
    hide();
}

void AnimatedWidget::paintEvent( QPaintEvent *event )
{
    Q_UNUSED( event );

    const int row_size = m_pixmap.width() / m_size;
    const int row = m_currentFrame / row_size;
    const int column = m_currentFrame % row_size;

    QPainter p( this );
    p.fillRect( rect(), Qt::transparent );
    p.drawPixmap( 0, 0, m_pixmap, column * m_size, row * m_size, m_size, m_size );
}

void AnimatedWidget::resizeEvent( QResizeEvent *event )
{
    Q_UNUSED( event );

    m_timer.stop();
    load();
}

void AnimatedWidget::timerEvent( QTimerEvent *event )
{
    if ( event->timerId() == m_timer.timerId() )
    {
        m_currentFrame++;
        if ( m_currentFrame == m_frames )
            m_currentFrame = 0;
        update();
    }
    QWidget::timerEvent( event );
}

void AnimatedWidget::load()
{
    // FIXME implement a better logic for the animation size
    m_size = 22;
    const QString path = KIconLoader::global()->iconPath( m_icon, -m_size );
    QPixmap pix( path );
    if ( pix.isNull() )
        return;

    if ( ( pix.width() % m_size != 0 ) || ( pix.height() % m_size != 0 ) )
        return;

    m_frames = ( pix.height() / m_size ) * ( pix.width() / m_size );
    m_pixmap = pix;
    m_currentFrame = 0;
}

#include "animatedwidget.moc"
