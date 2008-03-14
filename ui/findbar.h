/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _FINDBAR_H_
#define _FINDBAR_H_

#include <qwidget.h>

class QAction;
class SearchLineEdit;

namespace Okular {
class Document;
}

class FindBar
    : public QWidget
{
    Q_OBJECT

    public:
        explicit FindBar( Okular::Document * document, QWidget * parent = 0 );
        virtual ~FindBar();

        QString text() const;
        Qt::CaseSensitivity caseSensitivity() const;

        void focusAndSetCursor();

    public slots:
        void findNext();
        void findPrev();

    private slots:
        void caseSensitivityChanged();

    private:
        SearchLineEdit * m_text;
        QAction * m_caseSensitiveAct;
};


#endif
