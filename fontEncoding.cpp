// fontEncoding.cpp
//
// Part of KDVI - A DVI previewer for the KDE desktop environemt 
//
// (C) 2003 Stefan Kebekus
// Distributed under the GPL

#include <../config.h>
#ifdef HAVE_FREETYPE

#include <kdebug.h>
#include <kprocio.h>
#include <qfile.h>
#include <qstringlist.h>

#include "fontEncoding.h"

//#define DEBUG_FONTENC

fontEncoding::fontEncoding(const QString &encName)
{
#ifdef DEBUG_FONTENC
  kdDebug() << "fontEncoding( " << encName << " )" << endl;
#endif

  // Use kpsewhich to find the encoding file.
  KProcIO proc;
  QString encFileName;
  proc << "kpsewhich" << encName;
  proc.start(KProcess::Block);
  proc.readln(encFileName);
  encFileName = encFileName.stripWhiteSpace();
  
#ifdef DEBUG_FONTENC
  kdDebug() << "FileName of the encoding: " << encFileName << endl;
#endif  

  QFile file( encFileName );
  if ( file.open( IO_ReadOnly ) ) {
    // Read the file (excluding comments) into the QString variable
    // 'fileContent'
    QTextStream stream( &file );
    QString fileContent;
    while ( !stream.atEnd() ) 
      fileContent += stream.readLine().section('%', 0, 0); // line of text excluding '\n' until first '%'-sign
    file.close();
    
    fileContent = fileContent.stripWhiteSpace();
    
    // Find the name of the encoding
    encodingFullName = fileContent.section(' ', 0, 0).mid(1);
#ifdef DEBUG_FONTENC
    kdDebug() << "encodingFullName: " << encodingFullName << endl;
#endif
    
    fileContent = fileContent.section('[', 1, 1).section(']',0,0).simplifyWhiteSpace();
    QStringList glyphNameList = QStringList::split( '/', fileContent );
    
    int i = 0;
    for ( QStringList::Iterator it = glyphNameList.begin(); it != glyphNameList.end(); ++it ) {
      glyphNameVector[i] = (*it).simplifyWhiteSpace();
#ifdef DEBUG_FONTENC
      kdDebug() << i << ": " << glyphNameVector[i] << endl;
#endif
      i++;
    }
    
  }
}


#endif // HAVE_FREETYPE
