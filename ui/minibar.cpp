/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2006 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt / kde includes
#include <qapplication.h>
#include <qevent.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <QStyleOptionButton>
#include <QStylePainter>
#include <qvalidator.h>
#include <qpainter.h>
#include <kiconloader.h>
#include <kacceleratormanager.h>
#include <kdeversion.h>

// local includes
#include "core/document.h"
#include "minibar.h"

// [private widget] show progress
class ProgressWidget : public QWidget
{
    public:
        ProgressWidget( MiniBar * parent );
        void setProgress( float percentage );

    protected:
        void mouseMoveEvent( QMouseEvent * e );
        void mousePressEvent( QMouseEvent * e );
        void wheelEvent( QWheelEvent * e );
        void paintEvent( QPaintEvent * e );

    private:
        MiniBar * m_miniBar;
        float m_progressPercentage;
};

// [private widget] lineEdit for entering/validating page numbers
class PagesEdit : public QLineEdit
{
    public:
        PagesEdit( MiniBar * parent );
        void setPagesNumber( int pages );
        void setText( const QString & );

    protected:
        void focusInEvent( QFocusEvent * e );
        void focusOutEvent( QFocusEvent * e );
        void mousePressEvent( QMouseEvent * e );
        void wheelEvent( QWheelEvent * e );

    private:
        MiniBar * m_miniBar;
        bool m_eatClick;
        QString backString;
        QIntValidator * m_validator;
};

// [private widget] a flat qpushbutton that enlights on hover
class HoverButton : public QPushButton
{
    public:
        HoverButton( QWidget * parent );

    protected:
        void paintEvent( QPaintEvent * e );
        void enterEvent( QEvent * e );
        void leaveEvent( QEvent * e );
};


/** MiniBar **/

MiniBar::MiniBar( QWidget * parent, KPDFDocument * document )
    : QFrame( parent ), m_document( document ),
    m_currentPage( -1 )
{
    setObjectName( "miniBar" );
    // left spacer
    QHBoxLayout * horLayout = new QHBoxLayout( this );
    horLayout->setMargin( 2 );
    QSpacerItem * spacerL = new QSpacerItem( 20, 10, QSizePolicy::Expanding );
    horLayout->addItem( spacerL );

    // central 2r by 5c grid layout that contains all components
    QGridLayout * gridLayout = new QGridLayout();
    gridLayout->setMargin( 2 );
    gridLayout->setSpacing( 1 );
     // top spacer 6x6 px
//     QSpacerItem * spacerTop = new QSpacerItem( 6, 6, QSizePolicy::Fixed, QSizePolicy::Fixed );
//     gridLayout->addMultiCell( spacerTop, 0, 0, 0, 4 );
     // center progress widget
     m_progressWidget = new ProgressWidget( this );
     gridLayout->addWidget( m_progressWidget, 0, 0, 1, 5 );
     // bottom: left prev_page button
     m_prevButton = new HoverButton( this );
     m_prevButton->setIcon( SmallIconSet("1leftarrow") );
     gridLayout->addWidget( m_prevButton, 1, 0 );
     // bottom: left lineEdit (current page box)
     m_pagesEdit = new PagesEdit( this );
     gridLayout->addWidget( m_pagesEdit, 1, 1 );
     // bottom: central '/' label
     gridLayout->addWidget( new QLabel( "/", this ), 1, 2 );
     // bottom: right button
     m_pagesButton = new HoverButton( this );
     gridLayout->addWidget( m_pagesButton, 1, 3 );
     // bottom: right next_page button
     m_nextButton = new HoverButton( this );
     m_nextButton->setIcon( SmallIconSet("1rightarrow") );
     gridLayout->addWidget( m_nextButton, 1, 4 );
    horLayout->addLayout( gridLayout );

    // right spacer
    QSpacerItem * spacerR = new QSpacerItem( 20, 10, QSizePolicy::Expanding );
    horLayout->addItem( spacerR );

    // customize own look
    setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );

    // connect signals from child widgets to internal handlers / signals bouncers
    connect( m_pagesEdit, SIGNAL( returnPressed() ), this, SLOT( slotChangePage() ) );
    connect( m_pagesButton, SIGNAL( clicked() ), this, SIGNAL( gotoPage() ) );
    connect( m_prevButton, SIGNAL( clicked() ), this, SIGNAL( prevPage() ) );
    connect( m_nextButton, SIGNAL( clicked() ), this, SIGNAL( nextPage() ) );

    // widget starts hidden (will be shown after opening a document)
    parent->hide();
}

MiniBar::~MiniBar()
{
    m_document->removeObserver( this );
}

void MiniBar::notifySetup( const QVector< KPDFPage * > & pageVector, bool changed )
{
    // only process data when document changes
    if ( !changed )
        return;

    // if document is closed or has no pages, hide widget
    int pages = pageVector.count();
    if ( pages < 1 )
    {
        m_currentPage = -1;
        parentWidget()->hide();
        return;
    }

    // resize width of widgets
    int numberWidth = 10 + fontMetrics().width( QString::number( pages ) );
    m_pagesEdit->setMinimumWidth( numberWidth );
    m_pagesEdit->setMaximumWidth( 2 * numberWidth );
    m_pagesButton->setMinimumWidth( numberWidth );
    m_pagesButton->setMaximumWidth( 2 * numberWidth );

    // resize height of widgets
    int fixedHeight = fontMetrics().height() + 4;
    if ( fixedHeight < 20 )
        fixedHeight = 20;
    m_pagesEdit->setFixedHeight( fixedHeight );
    m_pagesButton->setFixedHeight( fixedHeight );
    m_prevButton->setFixedHeight( fixedHeight );
    m_nextButton->setFixedHeight( fixedHeight );

    // update child widgets
    m_pagesEdit->setPagesNumber( pages );
    m_pagesButton->setText( QString::number( pages ) );
    m_prevButton->setEnabled( false );
    m_nextButton->setEnabled( false );
    parentWidget()->show();
}

void MiniBar::notifyViewportChanged( bool /*smoothMove*/ )
{
    // get current page number
    int page = m_document->viewport().pageNumber;
    int pages = m_document->pages();

    // if the document is opened and page is changed
    if ( page != m_currentPage && pages > 0 )
    {
        // update percentage
        m_currentPage = page;
        float percentage = pages < 2 ? 1.0 : (float)page / (float)(pages - 1);
        m_progressWidget->setProgress( percentage );
        // update prev/next button state
        m_prevButton->setEnabled( page > 0 );
        m_nextButton->setEnabled( page < ( pages - 1 ) );
        // update text on widgets
        m_pagesEdit->setText( QString::number( page + 1 ) );
    }
}

void MiniBar::resizeEvent( QResizeEvent * e )
{
    // auto-hide 'prev' and 'next' buttons if not enough space
    const QSize & myHint = minimumSizeHint();
    bool shown = m_prevButton->isVisible() && m_nextButton->isVisible();
    if ( shown && e->size().width() < myHint.width() )
    {
        m_prevButton->hide();
        m_nextButton->hide();
        updateGeometry();
    }
    else if ( !shown )
    {
        int histeresis = m_prevButton->sizeHint().width() * 2 + 2;
        if ( e->size().width() > (myHint.width() + histeresis) )
        {
            m_prevButton->show();
            m_nextButton->show();
            updateGeometry();
        }
    }
}

void MiniBar::slotChangePage()
{
    // get text from the lineEdit
    QString pageNumber = m_pagesEdit->text();

    // convert it to page number and go to that page
    bool ok;
    int number = pageNumber.toInt( &ok ) - 1;
    if ( ok && number >= 0 && number < (int)m_document->pages() &&
         number != m_currentPage )
    {
        m_document->setViewportPage( number );
        m_pagesEdit->clearFocus();
    }
}

void MiniBar::slotGotoNormalizedPage( float index )
{
    // figure out page number and go to that page
    int number = (int)( index * (float)m_document->pages() );
    if ( number >= 0 && number < (int)m_document->pages() &&
         number != m_currentPage )
        m_document->setViewportPage( number );
}

void MiniBar::slotEmitNextPage()
{
    // emit signal
    nextPage();
}

void MiniBar::slotEmitPrevPage()
{
    // emit signal
    prevPage();
}



/** ProgressWidget **/

ProgressWidget::ProgressWidget( MiniBar * parent )
    : QWidget( parent ),
    m_miniBar( parent ), m_progressPercentage( -1 )
{
    setObjectName( "progress" );
    setAttribute( Qt::WA_OpaquePaintEvent, true );
    setFixedHeight( 4 );
    setMouseTracking( true );
}

void ProgressWidget::setProgress( float percentage )
{
    m_progressPercentage = percentage;
    update();
}

void ProgressWidget::mouseMoveEvent( QMouseEvent * e )
{
    if ( ( QApplication::mouseButtons() & Qt::LeftButton ) && width() > 0 )
        m_miniBar->slotGotoNormalizedPage( (float)e->x() / (float)width() );
}

void ProgressWidget::mousePressEvent( QMouseEvent * e )
{
    if ( e->button() == Qt::LeftButton && width() > 0 )
        m_miniBar->slotGotoNormalizedPage( (float)e->x() / (float)width() );
}

void ProgressWidget::wheelEvent( QWheelEvent * e )
{
    if ( e->delta() > 0 )
        m_miniBar->slotEmitNextPage();
    else
        m_miniBar->slotEmitPrevPage();
}

void ProgressWidget::paintEvent( QPaintEvent * e )
{
    if ( m_progressPercentage < 0.0 )
        return;

    // find out the 'fill' and the 'clear' rectangles
    int w = width(),
        h = height(),
        l = (int)( (float)w * m_progressPercentage );
    QRect cRect = QRect( l, 0, w - l, h ).intersect( e->rect() );
    QRect fRect = QRect( 0, 0, l, h ).intersect( e->rect() );
    QPainter p( this );

    QPalette pal = palette();
    // paint clear rect
    if ( cRect.isValid() )
        p.fillRect( cRect, pal.color( QPalette::Active, QPalette::HighlightedText ) );
    // draw a frame-like outline
    //p.setPen( palette().active().mid() );
    //p.drawRect( 0,0, w, h );
    // paint fill rect
    if ( fRect.isValid() )
        p.fillRect( fRect, pal.color( QPalette::Active, QPalette::Highlight ) );
    if ( l && l != w  )
    {
        p.setPen( pal.color( QPalette::Active, QPalette::Highlight ).dark( 120 ) );
        p.drawLine( l, 0, l, h );
    }
}


/** PagesEdit **/

PagesEdit::PagesEdit( MiniBar * parent )
    : QLineEdit( parent ), m_miniBar( parent ), m_eatClick( false )
{
    // customize look
#warning dont know how to port setFrameShadow
//    setFrameShadow( QFrame::Raised );

    // use an integer validator
    m_validator = new QIntValidator( 1, 1, this );
    setValidator( m_validator );

    // customize text properties
    setAlignment( Qt::AlignCenter );
    setMaxLength( 4 );

    // send a focus out event
    QFocusEvent fe( QEvent::FocusOut );
    QApplication::sendEvent( this, &fe );
}

void PagesEdit::setPagesNumber( int pages )
{
    m_validator->setTop( pages );
}

void PagesEdit::setText( const QString & text )
{
    // store a copy of the string
    backString = text;
    // call default handler if hasn't focus
    if ( !hasFocus() )
        QLineEdit::setText( text );
}

void PagesEdit::focusInEvent( QFocusEvent * e )
{
    // select all text
    selectAll();
    if ( e->reason() == Qt::MouseFocusReason )
        m_eatClick = true;
    // change background color to the default 'edit' color
#warning dont know how to port setLineWidth
//  setLineWidth( 2 );
    QPalette pal = palette();
    pal.setColor( QPalette::Active, QPalette::Base, QApplication::palette().color( QPalette::Active, QPalette::Base ) );
    setPalette( pal );
    // call default handler
    QLineEdit::focusInEvent( e );
}

void PagesEdit::focusOutEvent( QFocusEvent * e )
{
    // change background color to a dark tone
#warning dont know how to port setLineWidth
//  setLineWidth( 1 );
    QPalette pal = palette();
    pal.setColor( QPalette::Base, QApplication::palette().color( QPalette::Base ).dark( 102 ) );
    setPalette( pal );
    // restore text
    QLineEdit::setText( backString );
    // call default handler
    QLineEdit::focusOutEvent( e );
}

void PagesEdit::mousePressEvent( QMouseEvent * e )
{
    // if this click got the focus in, don't process the event
    if ( !m_eatClick )
        QLineEdit::mousePressEvent( e );
    m_eatClick = false;
}

void PagesEdit::wheelEvent( QWheelEvent * e )
{
    if ( e->delta() > 0 )
        m_miniBar->slotEmitNextPage();
    else
        m_miniBar->slotEmitPrevPage();
}


/** HoverButton **/

HoverButton::HoverButton( QWidget * parent )
    : QPushButton( parent )
{
    setMouseTracking( true );
    KAcceleratorManager::setNoAccel( this );
}

void HoverButton::enterEvent( QEvent * e )
{
	update();
	QPushButton::enterEvent( e );
}

void HoverButton::leaveEvent( QEvent * e )
{
	update();
	QPushButton::leaveEvent( e );
}

void HoverButton::paintEvent( QPaintEvent * e )
{
    if ( testAttribute( Qt::WA_UnderMouse ) && isEnabled() )
    {
        QPushButton::paintEvent( e );
    }
    else
    {
        QStylePainter p( this );
        QStyleOptionButton opt;
        opt.initFrom( this );
        opt.features = QStyleOptionButton::Flat;
        opt.icon = icon();
        opt.iconSize = iconSize();
        opt.text = text();
        p.drawControl( QStyle::CE_PushButton, opt );
    }
}

#include "minibar.moc"
