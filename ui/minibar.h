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

#include <qframe.h>
#include "core/observer.h"

namespace Okular {
class Document;
}

class PagesEdit;
class HoverButton;

/**
 * @short A widget to display page number and change current page.
 */
class MiniBar : public QFrame, public Okular::DocumentObserver
{
    Q_OBJECT
    public:
        MiniBar( QWidget *parent, Okular::Document * document );
        ~MiniBar();

        // [INHERITED] from DocumentObserver
        uint observerId() const { return MINIBAR_ID; }
        void notifySetup( const QVector< Okular::Page * > & pages, bool );
        void notifyViewportChanged( bool smoothMove );

    signals:
        void gotoPage();
        void prevPage();
        void nextPage();

    public slots:
        void slotChangePage();
        void slotEmitNextPage();
        void slotEmitPrevPage();

    private:
        Okular::Document * m_document;
        PagesEdit * m_pagesEdit;
        HoverButton * m_prevButton;
        HoverButton * m_pagesButton;
        HoverButton * m_nextButton;
        int m_currentPage;
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
        uint observerId() const { return PROGRESSWIDGET_ID; }
        void notifyViewportChanged( bool smoothMove );

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
        int m_currentPage;
        float m_progressPercentage;
};

#endif
