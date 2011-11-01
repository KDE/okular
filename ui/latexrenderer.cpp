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

#include "latexrenderer.h"

#include <kdebug.h>
#include <kstandarddirs.h>
#include <kprocess.h>
#include <ktemporaryfile.h>

#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QTextDocument>
#include <QTextStream>

namespace GuiUtils
{

LatexRenderer::LatexRenderer()
{
}

LatexRenderer::~LatexRenderer()
{
    foreach(const QString &file, m_fileList)
    {
        QFile::remove(file);
    }
}

LatexRenderer::Error LatexRenderer::renderLatexInHtml( QString& html, const QColor& textColor, int fontSize, int resolution, QString& latexOutput )
{
    if( !html.contains("$$"))
        return NoError;

    // this searches for $$formula$$ 
    QRegExp rg("\\$\\$.+\\$\\$");
    rg.setMinimal(true);

    int pos = 0;

    QMap<QString, QString> replaceMap;
    while (pos >= 0 && pos < html.length())
    {
        pos = rg.indexIn(html, pos);

        if (pos >= 0 )
        {
            const QString match = rg.cap(0);
            pos += rg.matchedLength();

            QString formul=match;
            // first remove the $$ delimiters on start and end
            formul.remove("$$");
            // then trim the result, so we can skip totally empty/whitespace-only formulas
            formul = formul.trimmed();
            if (formul.isEmpty() || !securityCheck(formul))
                continue;

            //unescape formula
            formul.replace("&gt;",">").replace("&lt;","<").replace("&amp;","&").replace("&quot;","\"").replace("&apos;","\'").replace("<br>"," ");

            QString fileName;
            Error returnCode = handleLatex(fileName, formul, textColor, fontSize, resolution, latexOutput);
            if (returnCode != NoError)
                return returnCode;

            replaceMap[match] = fileName;
        }
    }
    
    if(replaceMap.isEmpty()) //we haven't found any LaTeX strings
        return NoError;
    
    int imagePxWidth,imagePxHeight;
    for (QMap<QString,QString>::ConstIterator it = replaceMap.constBegin(); it != replaceMap.constEnd(); ++it)
    {
        QImage theImage(*it);
        if(theImage.isNull())
            continue;
        imagePxWidth = theImage.width();
        imagePxHeight = theImage.height();
        QString escapedLATEX=Qt::escape(it.key()).replace('\"',"&quot;");  //we need  the escape quotes because that string will be in a title="" argument, but not the \n
        html.replace(it.key(), " <img width=\"" + QString::number(imagePxWidth) + "\" height=\"" + QString::number(imagePxHeight) + "\" align=\"middle\" src=\"" + (*it) + "\"  alt=\"" + escapedLATEX +"\" title=\"" + escapedLATEX +"\"  /> ");
    }
    return NoError;
}

bool LatexRenderer::mightContainLatex (const QString& text)
{
    if( !text.contains("$$"))
        return false;

    // this searches for $$formula$$ 
    QRegExp rg("\\$\\$.+\\$\\$");
    rg.setMinimal(true);
    if( rg.lastIndexIn(text) == -1 )
        return false;

    return true;
}

LatexRenderer::Error LatexRenderer::handleLatex( QString& fileName, const QString& latexFormula, const QColor& textColor, int fontSize, int resolution, QString& latexOutput )
{
    KProcess latexProc;
    KProcess dvipngProc;

    KTemporaryFile *tempFile = new KTemporaryFile();
    tempFile->setPrefix("kdelatex-");
    tempFile->setSuffix(".tex");
    tempFile->open();
    QString tempFileName = tempFile->fileName();
    QFileInfo *tempFileInfo = new QFileInfo(tempFileName);
    QString tempFileNameNS = tempFileInfo->absolutePath() + QDir::separator() + tempFileInfo->baseName();
    QString tempFilePath = tempFileInfo->absolutePath();
    delete tempFileInfo;
    QTextStream tempStream(tempFile);

    tempStream << "\
\\documentclass[" << fontSize << "pt]{article} \
\\usepackage{color} \
\\usepackage{amsmath,latexsym,amsfonts,amssymb,ulem} \
\\pagestyle{empty} \
\\begin{document} \
{\\color[rgb]{" << textColor.redF() << "," << textColor.greenF() << "," << textColor.blueF() << "} \
\\begin{eqnarray*} \
" << latexFormula << " \
\\end{eqnarray*}} \
\\end{document}";

    tempFile->close();
    QString latexExecutable = KStandardDirs::findExe("latex");
    if (latexExecutable.isEmpty())
    {
        kDebug() << "Could not find latex!";
        delete tempFile;
        fileName = QString();
        return LatexNotFound;
    }
    latexProc << latexExecutable << "-interaction=nonstopmode" << "-halt-on-error" << QString("-output-directory=%1").arg(tempFilePath) << tempFile->fileName();
    latexProc.setOutputChannelMode( KProcess::MergedChannels );
    latexProc.execute();
    latexOutput = latexProc.readAll();
    tempFile->remove();

    QFile::remove(tempFileNameNS + QString(".log"));
    QFile::remove(tempFileNameNS + QString(".aux"));
    delete tempFile;

    if (!QFile::exists(tempFileNameNS + QString(".dvi")))
    {
        fileName = QString();
        return LatexFailed;
    }

    QString dvipngExecutable = KStandardDirs::findExe("dvipng");
    if (dvipngExecutable.isEmpty())
    {
        kDebug() << "Could not find dvipng!";
        fileName = QString();
        return DvipngNotFound;
    }

    dvipngProc << dvipngExecutable << QString("-o%1").arg(tempFileNameNS + QString(".png")) << "-Ttight" << "-bgTransparent" << QString("-D %1").arg(resolution) << QString("%1").arg(tempFileNameNS + QString(".dvi"));
    dvipngProc.setOutputChannelMode( KProcess::MergedChannels );
    dvipngProc.execute();

    QFile::remove(tempFileNameNS + QString(".dvi"));
    
    if (!QFile::exists(tempFileNameNS + QString(".png")))
    {
        fileName = QString();
        return DvipngFailed;
    }

    fileName = tempFileNameNS + QString(".png");
    m_fileList << fileName;
    return NoError;
}

bool LatexRenderer::securityCheck( const QString &latexFormula )
{
    return !latexFormula.contains(QRegExp("\\\\(def|let|futurelet|newcommand|renewcomment|else|fi|write|input|include"
    "|chardef|catcode|makeatletter|noexpand|toksdef|every|errhelp|errorstopmode|scrollmode|nonstopmode|batchmode"
    "|read|csname|newhelp|relax|afterground|afterassignment|expandafter|noexpand|special|command|loop|repeat|toks"
    "|output|line|mathcode|name|item|section|mbox|DeclareRobustCommand)[^a-zA-Z]"));
}

}
