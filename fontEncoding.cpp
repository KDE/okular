// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// fontEncoding.cpp
//
// Part of KDVI - A DVI previewer for the KDE desktop environment
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#include <config.h>

#ifdef HAVE_FREETYPE

#include "fontEncoding.h"
#include "kvs_debug.h"

#include <QFile>
#include <QProcess>
#include <QStringList>
#include <QTextStream>

//#define DEBUG_FONTENC


fontEncoding::fontEncoding(const QString &encName)
{
#ifdef DEBUG_FONTENC
  kdDebug(kvs::dvi) << "fontEncoding( " << encName << " )" << endl;
#endif

  _isValid = false;
  // Use kpsewhich to find the encoding file.
  QProcess kpsewhich;
  kpsewhich.setReadChannelMode(QProcess::MergedChannels);

  kpsewhich.start("kpsewhich",
                  QStringList() << encName,
                  QIODevice::ReadOnly|QIODevice::Text);

  if (!kpsewhich.waitForStarted()) {
    kdError(kvs::dvi) << "fontEncoding::fontEncoding(...): kpsewhich could not be started." << endl;
    return;
  }

  // We wait here while the external program runs concurrently.
  kpsewhich.waitForFinished(-1);

  const QString encFileName = QString(kpsewhich.readAll()).trimmed();
  if (encFileName.isEmpty()) {
    kdError(kvs::dvi) << QString("fontEncoding::fontEncoding(...): The file '%1' could not be found by kpsewhich.").arg(encName) << endl;
    return;
  }

#ifdef DEBUG_FONTENC
  kdDebug(kvs::dvi) << "FileName of the encoding: " << encFileName << endl;
#endif

  QFile file( encFileName );
  if ( file.open( QIODevice::ReadOnly ) ) {
    // Read the file (excluding comments) into the QString variable
    // 'fileContent'
    QTextStream stream( &file );
    QString fileContent;
    while ( !stream.atEnd() )
      fileContent += stream.readLine().section('%', 0, 0); // line of text excluding '\n' until first '%'-sign
    file.close();

    fileContent = fileContent.trimmed();

    // Find the name of the encoding
    encodingFullName = fileContent.section('[', 0, 0).simplified().mid(1);
#ifdef DEBUG_FONTENC
    kdDebug(kvs::dvi) << "encodingFullName: " << encodingFullName << endl;
#endif

    fileContent = fileContent.section('[', 1, 1).section(']',0,0).simplified();
    QStringList glyphNameList = QStringList::split( '/', fileContent );

    int i = 0;
    for ( QStringList::Iterator it = glyphNameList.begin(); (it != glyphNameList.end())&&(i<256); ++it ) {
      glyphNameVector[i] = (*it).simplified();
#ifdef DEBUG_FONTENC
      kdDebug(kvs::dvi) << i << ": " << glyphNameVector[i] << endl;
#endif
      i++;
    }
    for(; i<256; i++)
      glyphNameVector[i] = ".notdef";
  } else {
    kdError(kvs::dvi) << QString("fontEncoding::fontEncoding(...): The file '%1' could not be opened.").arg(encFileName) << endl;
    return;
  }

  _isValid = true;
}

#endif // HAVE_FREETYPE
