/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2006 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_MINIBAR_H_
#define _OKULAR_MINIBAR_H_

#include <qset.h>
#include <qwidget.h>
#include <klineedit.h>
#include "core/observer.h"

namespace Okular {
class Document;
}

class MiniBar;
class HoverButton;
class QIntValidator;
class QLabel;
class QToolBar;

// [private widget] lineEdit for entering/validating page numbers
class PagesEdit : public KLineEdit
{
    Q_OBJECT

    public:
        PagesEdit( MiniBar * parent );
        void setText( const QString & );

    protected:
        void focusInEvent( QFocusEvent * e );
        void focusOutEvent( QFocusEvent * e );
        void mousePressEvent( QMouseEvent * e );
        void wheelEvent( QWheelEvent * e );

    private slots:
        void updatePalette();

    private:
        MiniBar * m_miniBar;
        bool m_eatClick;
};

class PageNumberEdit : public PagesEdit
{
    public:
        PageNumberEdit( MiniBar * parent );
        void setPagesNumber( int pages );

    private:
        QIntValidator * m_validator;
};

class PageLabelEdit : public PagesEdit
{
  Q_OBJECT
    public:
        PageLabelEdit( MiniBar * parent );
        void setText( const QString & newText );
        void setPageLabels( const QVector< Okular::Page * > & pageVector );

    signals:
        void pageNumberChosen( int page );

    private slots:
        void pageChosen();

    private:
        QString m_lastLabel;
        QMap<QString, int> m_labelPageMap;
};

/**
 * @short The object that observes the document and feeds the minibars
 */
class MiniBarLogic : public QObject, public Okular::DocumentObserver
{
    public:
        MiniBarLogic( QObject * parent, Okular::Document * m_document );
        ~MiniBarLogic();
        
        void addMiniBar( MiniBar * miniBar );
        void removeMiniBar( MiniBar * miniBar );
        
        Okular::Document *document() const;
        int currentPage() const;
        
        // [INHERITED] from DocumentObserver
        void notifySetup( const QVector< Okular::Page * > & pages, int setupFlags );
        void notifyCurrentPageChanged( int previous, int current );
        
    private:
        QSet<MiniBar *> m_miniBars;
        Okular::Document * m_document;
};

/**
 * @short A widget to display page number and change current page.
 */
class MiniBar : public QWidget
{
    Q_OBJECT
    friend class MiniBarLogic;
    
    public:
        MiniBar( QWidget *parent, MiniBarLogic * miniBarLogic );
        ~MiniBar();

        void changeEvent( QEvent * event ) ;

    signals:
        void gotoPage();
        void prevPage();
        void nextPage();
        void forwardKeyPressEvent( QKeyEvent *e );

    public slots:
        void slotChangePage();
        void slotChangePage(int page);
        void slotEmitNextPage();
        void slotEmitPrevPage();
        void slotToolBarIconSizeChanged();

    private:
        void resizeForPage( int pages );
        bool eventFilter( QObject *target, QEvent *event );

        MiniBarLogic * m_miniBarLogic;
        PageNumberEdit * m_pageNumberEdit;
        PageLabelEdit * m_pageLabelEdit;
        QLabel * m_pageNumberLabel;
        HoverButton * m_prevButton;
        HoverButton * m_pagesButton;
        HoverButton * m_nextButton;
        QToolBar * m_oldToobarParent;
};

/**
 * @short A small progress bar.
 */
class ProgressWidget : public QWidget, public Okular::DocumentObserver
{
    Q_OBJECT
    public:
        ProgressWidget( QWidget * parent, Okular::Document * document );
        ~ProgressWidget();

        // [INHERITED] from DocumentObserver
        void notifyCurrentPageChanged( int previous, int current );

        void slotGotoNormalizedPage( float index );

    signals:
        void prevPage();
        void nextPage();

    protected:
        void setProgress( float percentage );

        void mouseMoveEvent( QMouseEvent * e );
        void mousePressEvent( QMouseEvent * e );
        void wheelEvent( QWheelEvent * e );
        void paintEvent( QPaintEvent * e );

    private:
        Okular::Document * m_document;
        float m_progressPercentage;
};

#endif
