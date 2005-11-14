// -*- C++ -*-
//
// C++ Interface: dvisourcesplitter
//
// Author: Jeroen Wijnhout <Jeroen.Wijnhout@kdemail.net>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//

#ifndef DVI_SOURCEFILESPLITTER_H
#define DVI_SOURCEFILESPLITTER_H

#include <QFileInfo>

class QString;


class DVI_SourceFileSplitter
{
public:
  DVI_SourceFileSplitter(const QString & scrlink, const QString & dviFile);

  QString  fileName() { return m_fileInfo.fileName(); }
  QString  filePath() { return m_fileInfo.absoluteFilePath(); }
  bool     fileExists() { return m_fileInfo.exists(); }

  quint32 line()     { return m_line; }

private:
  QFileInfo m_fileInfo;
  quint32  m_line;
  bool      m_exists;
};
#endif
