/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_SEARCHWIDGET_H_
#define _KPDF_SEARCHWIDGET_H_

#include <ktoolbar.h>

class KPopupMenu;
class KPDFDocument;
class m_inputDelayTimer;

// definition of searchID for this class (publicly available to ThumbnailsList)
#define SW_SEARCH_ID 3

/**
 * @short A widget for find-as-you-type search. Outputs to the Document.
 *
 * This widget accepts keyboard input and performs a call to findTextAll(..)
 * in the KPDFDocument class when there are 3 or more chars to search for.
 * It supports case sensitive/unsensitive(default) and provieds a button
 * for switching between the 2 modes.
 */
class SearchWidget : public KToolBar
{
    Q_OBJECT
    public:
        SearchWidget( QWidget *parent, KPDFDocument *document );
        void clearText();

    private:
        KPDFDocument * m_document;
        KPopupMenu * m_menu;
        QTimer * m_inputDelayTimer;
        int m_searchType;
        bool m_caseSensitive;

    private slots:
        void slotTextChanged( const QString & text );
        void slotMenuChaged( int index );
        void startSearch();
};

#endif
