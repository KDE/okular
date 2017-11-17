/***************************************************************************
 *   Copyright (C) 2006 by Chu Xiaodong <xiaodongchu@gmail.com>            *
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2017    Klarälvdalens Datakonsult AB, a KDAB Group      *
 *                         company, info@kdab.com. Work sponsored by the   *
 *                         LiMux project of the city of Munich             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _ANNOTWINDOW_H_
#define _ANNOTWINDOW_H_

#include <qcolor.h>
#include <qframe.h>

namespace Okular {
class Annotation;
class Document;
}

namespace GuiUtils {
class LatexRenderer;
}

class KTextEdit;
class MovableTitle;
class QMenu;

class AnnotWindow : public QFrame
{
    Q_OBJECT
    public:
        AnnotWindow( QWidget * parent, Okular::Annotation * annot, Okular::Document * document, int page );
        ~AnnotWindow();

        void reloadInfo();

        Okular::Annotation * annotation() const;
        int pageNumber() const;

        void updateAnnotation( Okular::Annotation * a );

    private:
        MovableTitle * m_title;
        KTextEdit *textEdit;
        QColor m_color;
        GuiUtils::LatexRenderer *m_latexRenderer;
        Okular::Annotation* m_annot;
        Okular::Document* m_document;
        int m_page;
        int m_prevCursorPos;
        int m_prevAnchorPos;

    protected:
        void showEvent( QShowEvent * event ) override;
        bool eventFilter( QObject * obj, QEvent * event ) override;

    private Q_SLOTS:
        void slotUpdateUndoAndRedoInContextMenu(QMenu *menu);
        void slotOptionBtn();
        void slotsaveWindowText();
        void renderLatex( bool render );
        void slotHandleContentsChangedByUndoRedo( Okular::Annotation* annot, QString contents, int cursorPos, int anchorPos);

    Q_SIGNALS:
        void containsLatex( bool );
};


#endif
