
#include <kdebug.h>
#include <kprocess.h>
#include <qapplication.h>

#include "fontpool.h"


// List of permissible MetaFontModes which are supported by kdvi.

const char *MFModes[]       = { "cx", "ljfour", "lexmarks" };
const char *MFModenames[]   = { "Canon CX", "LaserJet 4", "Lexmark S" };
int         MFResolutions[] = { 300, 600, 1200 };
 

unsigned int fontPool::setMetafontMode( unsigned int mode )
{
  if (mode >= NumberOfMFModes) {
    kdDebug() << "fontPool::setMetafontMode called with argument " << mode 
	      << " which is more than the allowed value of " << NumberOfMFModes-1 << endl;
    kdDebug() << "setting mode to " << MFModes[DefaultMFMode] << " at " 
	      << MFResolutions[DefaultMFMode] << "dpi" << endl;
    mode = DefaultMFMode;
  }
  MetafontMode = mode;
  return mode;
}

void fontPool::setMakePK(int flag)
{
  makepk = flag;
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

  // Just make sure that MetafontMode is in the permissible range, so
  // as to avoid core dumps.
  if (MetafontMode >= NumberOfMFModes) {
    kdError() << "fontPool::appendx called with bad MetafontMode " << MetafontMode 
	      << " which is more than the allowed value of " << NumberOfMFModes-1 << endl;
    kdError() << "setting mode to " << MFModes[DefaultMFMode] << " at " 
	      << MFResolutions[DefaultMFMode] << "dpi" << endl;
    MetafontMode = DefaultMFMode;
  }

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
    proc << QString("--dpi %1").arg(MFResolutions[MetafontMode]);
    proc << QString("--mode %1").arg(MFModes[MetafontMode]);
    proc << "--format pk";
    if (makepk == 0)
      proc << "--no-mktex pk";
    else
      proc << "--mktex pk";
    proc << QString("%1.%2").arg(fontp->fontname).arg((int)(fontp->fsize + 0.5));
 
    kdDebug() << "kpsewhich"
	      << QString(" --dpi %1").arg(MFResolutions[MetafontMode])
	      << QString(" --mode %1").arg(MFModes[MetafontMode])
	      << " --format pk"
	      << QString(" %1.%2").arg(fontp->fontname).arg((int)(fontp->fsize + 0.5))
	      << endl;
 

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
