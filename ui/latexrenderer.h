/***************************************************************************
 *   Copyright (C) 2004 by Duncan Mac-Vicar Prett <duncan@kde.org>         *
 *   Copyright (C) 2004-2005 by Olivier Goffart <ogoffart@kde.org>         *
 *   Copyright (C) 2011 by Niels Ole Salscheider                           *
 *                         <niels_ole@salscheider-online.de>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef LATEXRENDERER_H
#define LATEXRENDERER_H

#include <QStringList>

class QString;
class QColor;

namespace GuiUtils
{
class LatexRenderer
{
public:
    enum Error { NoError, LatexNotFound, DvipngNotFound, LatexFailed, DvipngFailed };

    LatexRenderer();
    ~LatexRenderer();

    LatexRenderer(const LatexRenderer &) = delete;
    LatexRenderer &operator=(const LatexRenderer &) = delete;

    Error renderLatexInHtml(QString &html, const QColor &textColor, int fontSize, int resolution, QString &latexOutput);
    static bool mightContainLatex(const QString &text);

private:
    Error handleLatex(QString &fileName, const QString &latexFormula, const QColor &textColor, int fontSize, int resolution, QString &latexOutput);
    static bool securityCheck(const QString &latexFormula);

    QStringList m_fileList;
};

}

#endif // LATEXRENDERER_H
