/*
    SPDX-FileCopyrightText: 2004 Duncan Mac-Vicar Prett <duncan@kde.org>
    SPDX-FileCopyrightText: 2004-2005 Olivier Goffart <ogoffart@kde.org>
    SPDX-FileCopyrightText: 2011 Niels Ole Salscheider
    <niels_ole@salscheider-online.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
