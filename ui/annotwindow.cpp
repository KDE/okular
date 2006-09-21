/***************************************************************************
 *   Copyright (C) 2006 by Chu Xiaodong <xiaodongchu@gmail.com>            *
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qapplication.h>
#include <qevent.h>
#include <qfont.h>
#include <qfontinfo.h>
#include <qfontmetrics.h>
#include <qframe.h>
#include <qlabel.h>
#include <qlayout.h>
#include <QPainter>
#include <qpushbutton.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qstylepainter.h>
#include <qtextedit.h>
#include <qtoolbutton.h>
#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>

// local includes
#include "annotwindow.h"
#include "core/annotations.h"


class CloseButton
  : public QPushButton
{
public:
    CloseButton( QWidget * parent = 0 )
      : QPushButton( parent )
    {
        setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
        QSize size = QSize( 14, 14 ).expandedTo( QApplication::globalStrut() );
        setFixedSize( size );
        setIcon( style()->standardIcon( QStyle::SP_DockWidgetCloseButton ) );
        setIconSize( size );
        setToolTip( i18n( "Close" ) );
    }
};


class MovableTitle
  : public QWidget
{
public:
    MovableTitle( QWidget * parent )
      : QWidget( parent )
    {
        QVBoxLayout * mainlay = new QVBoxLayout( this );
        mainlay->setMargin( 0 );
        mainlay->setSpacing( 0 );
        // close button row
        QHBoxLayout * buttonlay = new QHBoxLayout();
        mainlay->addLayout( buttonlay );
        titleLabel = new QLabel( this );
        QFont f = titleLabel->font();
        f.setBold( true );
        titleLabel->setFont( f );
        titleLabel->setCursor( Qt::SizeAllCursor );
        buttonlay->addWidget( titleLabel );
        dateLabel = new QLabel( this );
        dateLabel->setAlignment( Qt::AlignTop | Qt::AlignRight );
        f = dateLabel->font();
        f.setPointSize( QFontInfo( f ).pointSize() - 2 );
        dateLabel->setFont( f );
        dateLabel->setCursor( Qt::SizeAllCursor );
        buttonlay->addWidget( dateLabel );
        CloseButton * close = new CloseButton( this );
        connect( close, SIGNAL( clicked() ), parent, SLOT( hide() ) );
        buttonlay->addWidget( close );
        // option button row
        QHBoxLayout * optionlay = new QHBoxLayout();
        mainlay->addLayout( optionlay );
        authorLabel = new QLabel( this );
        authorLabel->setCursor( Qt::SizeAllCursor );
        authorLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
        optionlay->addWidget( authorLabel );
        optionButton = new QToolButton( this );
        QString opttext = i18n( "Options" );
        optionButton->setText( opttext );
        optionButton->setAutoRaise( true );
        QSize s = QFontMetrics( optionButton->font() ).boundingRect( opttext ).size() + QSize( 8, 8 );
        optionButton->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
        optionButton->setFixedSize( s );
        optionlay->addWidget( optionButton );

        titleLabel->installEventFilter( this );
        dateLabel->installEventFilter( this );
        authorLabel->installEventFilter( this );
    }

    virtual bool eventFilter( QObject * obj, QEvent * e )
    {
        if ( obj != titleLabel && obj != authorLabel && obj != dateLabel )
            return false;

        QMouseEvent * me = 0;
        switch ( e->type() )
        {
            case QEvent::MouseButtonPress:
                me = (QMouseEvent*)e;
                mousePressPos = me->pos();
                break;
            case QEvent::MouseButtonRelease:
                mousePressPos = QPoint();
                break;
            case QEvent::MouseMove:
                me = (QMouseEvent*)e;
                parentWidget()->move( me->pos() - mousePressPos + parentWidget()->pos() );
                break;
            default:
                return false;
        }
        return true;
    }

    void setTitle( const QString& title )
    {
        titleLabel->setText( QString( " " ) + title );
    }

    void setDate( const QDateTime& dt )
    {
        dateLabel->setText( KGlobal::locale()->formatDateTime( dt, true, true ) + ' ' );
    }

    void setAuthor( const QString& author )
    {
        authorLabel->setText( QString( " " ) + author );
    }

    void connectOptionButton( QObject * recv, const char* method )
    {
        connect( optionButton, SIGNAL( clicked() ), recv, method );
    }

private:
    QLabel * titleLabel;
    QLabel * dateLabel;
    QLabel * authorLabel;
    QPoint mousePressPos;
    QToolButton * optionButton;
};


class ResizeBox
  : public QWidget
{
public:
    ResizeBox( QWidget * parent )
      : QWidget( parent )
    {
        setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
        setFixedSize( style()->sizeFromContents( QStyle::CT_SizeGrip, 0, QSize( 11, 11 ) ) );
        setCursor( layoutDirection() == Qt::LeftToRight ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor );
    }

    virtual void paintEvent( QPaintEvent * )
    {
        QStyleOptionSizeGrip opt;
        opt.initFrom( this );
        opt.corner = layoutDirection() == Qt::LeftToRight ? Qt::BottomRightCorner : Qt::BottomLeftCorner;
        QStylePainter sp( this );
        sp.drawControl( QStyle::CE_SizeGrip, opt );
    }

    virtual void mousePressEvent( QMouseEvent * e )
    {
        mousePressPos = e->pos();
    }

    virtual void mouseMoveEvent( QMouseEvent * e )
    {
        QSize sz = parentWidget()->size();
        QPoint dpt = e->pos() - mousePressPos;
        sz.setHeight( qMax( parentWidget()->minimumHeight(), sz.height() + dpt.y() ) );
        sz.setWidth( qMax( parentWidget()->minimumWidth(), sz.width() + dpt.x() ) );
        parentWidget()->resize( sz );
    }

private:
    QPoint mousePressPos;
};


AnnotWindow::AnnotWindow( QWidget * parent, Okular::Annotation * annot)
    : QFrame( parent ), m_annot( annot )
{
    setAutoFillBackground( true );
    setFrameStyle( Panel | Raised );

    textEdit=new QTextEdit(m_annot->window.text, this);
    connect(textEdit,SIGNAL(textChanged()),
            this,SLOT(slotsaveWindowText()));
    
    QColor col = m_annot->style.color.isValid() ? m_annot->style.color : Qt::yellow;
    setPalette( QPalette( col ) );
    QPalette pl=textEdit->palette();
    pl.setColor( QPalette::Base, col );
    textEdit->setPalette(pl);

    QVBoxLayout * mainlay = new QVBoxLayout( this );
    mainlay->setMargin( 2 );
    mainlay->setSpacing( 0 );
    m_title = new MovableTitle( this );
    mainlay->addWidget( m_title );
    mainlay->addWidget( textEdit );
    QHBoxLayout * lowerlay = new QHBoxLayout();
    mainlay->addLayout( lowerlay );
    lowerlay->addItem( new QSpacerItem( 5, 5, QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    ResizeBox * sb = new ResizeBox( this );
    lowerlay->addWidget( sb );

    m_title->setTitle( m_annot->window.summary );
    m_title->setDate( m_annot->modifyDate );
    m_title->setAuthor( m_annot->author );
    m_title->connectOptionButton( this, SLOT( slotOptionBtn() ) );

    setGeometry(10,10,300,300 );
}

void AnnotWindow::slotOptionBtn()
{
    //TODO: call context menu in pageview
    //emit sig...
}

 void AnnotWindow::slotsaveWindowText()
{
    m_annot->window.text=textEdit->toPlainText();
}

#include "annotwindow.moc"
