
#include <kdebug.h>
#include <kprocess.h>
#include <qapplication.h>

#include "fontpool.h"

void fontPool::setResolution( int basedpi )
{
  Resolution = basedpi;
}

void fontPool::setMetafontMode( const QString &mode )
{
  MetafontMode = mode;
}

void fontPool::setMakePK(int flag)
{
  makepk = flag;
  kdDebug() << "MakeTexPK " << flag << endl;
}

void fontPool::appendx(const struct font *fontp)
{
  this->append(fontp);

  // Find the font name. We use the external program "kpsewhich" for
  // that purpose.

  // Our way of running the process is, of course, a terrible
  // hack. Currently, it takes 50% of CPU time just to check if the
  // Process is still alive. I will change that as soon as
  // possible. ---Stefan Kebekus.

  KShellProcess proc;
  qApp->connect(&proc, SIGNAL(receivedStdout(KProcess *, char *, int)),
		fontp, SLOT(font_name_receiver(KProcess *, char *, int)));
  
  // First try if the font is a virtual font
  proc.clearArguments();
  proc << "kpsewhich";
  proc << "--format vf";
  proc << fontp->fontname;
  proc.start(KProcess::NotifyOnExit, KProcess::All);
  while(proc.isRunning())
    qApp->processEvents();
  
  // Font not found? Then check if the font is a regular font.
  if (fontp->filename.isEmpty()) {
    proc.clearArguments();
    proc << "kpsewhich";
    proc << QString("--dpi %1").arg(Resolution);
    proc << QString("--mode %1").arg(MetafontMode);
    proc << "--format pk";
    if (makepk == 0)
      proc << "--no-mktex pk";
    else
      proc << "--mktex pk";
    proc << QString("%1.%2").arg(fontp->fontname).arg((int)(fontp->fsize + 0.5));
    proc.start(KProcess::NotifyOnExit, KProcess::All);
    while(proc.isRunning())
      qApp->processEvents();
  }
}


void fontPool::status(void)
{
  struct font *fontp;

  for ( fontp=this->first(); fontp != 0; fontp=this->next() ) 
    kdDebug() << fontp->fontname << ": " << fontp->flags << endl;
}
