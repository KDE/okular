// fontMap.cpp
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#include <../config.h>
#ifdef HAVE_FREETYPE

#include <kdebug.h>
#include <kprocio.h>
#include <kstringhandler.h>
#include <qfile.h>

#include "fontMap.h"

fontMap::fontMap()
{
  // Read the map file of ps2pk which will provide us with a
  // dictionary "TeX Font names" <-> "Name of font files, Font Names
  // and Encodings" (example: the font "Times-Roman" is called
  // "ptmr8y" in the DVI file, but the Type1 font file name is
  // "utmr8a.pfb". We use the map file of "ps2pk" because that progam
  // has, like kdvi (and unlike dvips), no built-in fonts.
  
  KProcIO proc;
  proc << "kpsewhich" << "--format=dvips config" << "ps2pk.map";
  if (proc.start(KProcess::Block) == false) {
    kdError(4700) << "fontMap::fontMap(): kpsewhich could not be started." << endl;
    return;
  }

  QString map_fileName;
  proc.readln(map_fileName);
  map_fileName = map_fileName.stripWhiteSpace();
  if (map_fileName.isEmpty()) {
    kdError(4700) << "fontMap::fontMap(): The file 'ps2pk.map' could not be found by kpsewhich." << endl;
    return;
  }
  
  QFile file( map_fileName );
  if ( file.open( IO_ReadOnly ) ) {
    QTextStream stream( &file );
    QString line;
    while ( !stream.atEnd() ) {
      line = stream.readLine().stripWhiteSpace();
      if (line.at(0) == '%')
	continue;
      
      QString TeXName  = KStringHandler::word(line, (unsigned int)0);
      QString fontFileName = line.section('<', -1);
      QString encodingName = line.section('<', -2, -2).stripWhiteSpace();
      
      fontMapEntry &entry = fontMapEntries[TeXName];

      entry.fontFileName = fontFileName;
      if (encodingName.endsWith(".enc"))
	entry.fontEncoding = encodingName;
    }
    file.close();
  } else {
    kdError(4700) << QString("fontMap::fontMap(): The file '%1' could not be opened.").arg(map_fileName) << endl;
    return;
  }
}


const QString &fontMap::findFileName(const QString &TeXName)
{
  QMap<QString, fontMapEntry>::Iterator it = fontMapEntries.find(TeXName);

  if (it != fontMapEntries.end())
    return it.data().fontFileName;
  else
    return QString::null;
}


const QString &fontMap::findEncoding(const QString &TeXName)
{
  QMap<QString, fontMapEntry>::Iterator it = fontMapEntries.find(TeXName);

  if (it != fontMapEntries.end())
    return it.data().fontEncoding;
  else
    return QString::null;
}

#endif // HAVE_FREETYPE
