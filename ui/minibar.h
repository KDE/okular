/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2006 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_MINIBAR_H_
#define _KPDF_MINIBAR_H_

#include <qframe.h>
#include "core/observer.h"

class KPDFDocument;
class PagesEdit;
class HoverButton;
class ProgressWidget;

/**
 * @short A widget to display page number and change current page.
 */
class MiniBar : public QFrame, public DocumentObserver
{
    Q_OBJECT
    public:
        MiniBar( QWidget *parent, KPDFDocument * document );
        ~MiniBar();

        // [INHERITED] from DocumentObserver
        uint observerId() const { return MINIBAR_ID; }
        void notifySetup( const QValueVector< KPDFPage * > & pages, bool );
        void notifyViewportChanged( bool smoothMove );

    signals:
        void gotoPage();
        void prevPage();
        void nextPage();

    public slots:
        void slotChangePage();
        void slotGotoNormalizedPage( float normIndex );
        void slotEmitNextPage();
        void slotEmitPrevPage();

    protected:
        void resizeEvent( QResizeEvent * );

    private:
        KPDFDocument * m_document;
        PagesEdit * m_pagesEdit;
        HoverButton * m_prevButton;
        HoverButton * m_pagesButton;
        HoverButton * m_nextButton;
        ProgressWidget * m_progressWidget;
        int m_currentPage;
};

#endif
