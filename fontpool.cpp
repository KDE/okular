
#include <kdebug.h>
#include <klocale.h>
#include <kprocess.h>
#include <qapplication.h>
#include <stdlib.h>

#include "fontpool.h"
#include "kdvi.h"

// List of permissible MetaFontModes which are supported by kdvi.

const char *MFModes[]       = { "cx", "ljfour", "lexmarks" };
const char *MFModenames[]   = { "Canon CX", "LaserJet 4", "Lexmark S" };
const int   MFResolutions[] = { 300, 600, 1200 };
 

fontPool::fontPool(void)
{
  proc = 0;
  fontList.setAutoDelete(TRUE);
}

unsigned int fontPool::setMetafontMode( unsigned int mode )
{
  if (mode >= NumberOfMFModes) {
    kdError() << "fontPool::setMetafontMode called with argument " << mode 
	      << " which is more than the allowed value of " << NumberOfMFModes-1 << endl;
    kdError() << "setting mode to " << MFModes[DefaultMFMode] << " at " 
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


class font *fontPool::appendx(char *fontname, float fsize, long checksum, int magstepval, double dconv)
{
  class font *fontp;

  // reuse font if possible
  for ( fontp=fontList.first(); fontp != 0; fontp=fontList.next() ) {
    if (strcmp(fontname, fontp->fontname) == 0 && (int (fsize+0.5)) == (int)(fontp->fsize + 0.5)) {
      // if font is already in the list
      fontp->mark_as_used();
      free(fontname);
      return fontp;
    }
  }

  // if font doesn't exist yet
  fontp = new font(fontname, fsize, checksum, magstepval, dconv, this);
  if (fontp == 0) {
    kdError() << i18n("Could not allocate memory for a font structure!") << endl;
    exit(0);
  }

  fontList.append(fontp);

  // Now start kpsewhich/MetaFont, etc. if necessary
  check_if_fonts_are_loaded();
  return fontp;
}


void fontPool::status(void)
{
  struct font *fontp;

  // Replace later by a better-looking table.
  for ( fontp=fontList.first(); fontp != 0; fontp=fontList.next() ) {
    QString entry = QString(" Font '%1'").arg(fontp->fontname);
    if (fontp->flags & font::FONT_IN_USE)
      entry.append(i18n(" is in use,"));
    else
      entry.append(i18n(" is not in use,"));

    if (fontp->flags & font::FONT_LOADED)
      entry.append(i18n(" has been loaded,"));
    else
      entry.append(i18n(" has not been loaded,"));

    if (fontp->flags & font::FONT_VIRTUAL)
      entry.append(i18n(" is a 'virtual font',"));
    else
      entry.append(i18n(" is a regular 'pk' font,"));

    if (fontp->flags & font::FONT_KPSE_NAME)
      entry.append(i18n(" filename has been looked up"));
    else
      entry.append(i18n(" filename has not (yet) been looked up"));

#ifdef DEBUG_FONTPOOL
    kdDebug() << entry << endl;
#endif
  }
}


char fontPool::check_if_fonts_are_loaded(void) 
{
#ifdef DEBUG_FONTPOOL
  kdDebug() << "Check if fonts have been looked for..." << endl;
#endif

  // Check if kpsewhich is still running. In that case there certainly
  // not all fonts have been properly looked up.
  if (proc != 0) {
#ifdef DEBUG_FONTPOOL
    kdDebug() << "... no, kpsewhich is still running." << endl;
#endif
    return -1;
  }
  
  // Is there a font whose name we did not try to find out yet?
  for (struct font *fontp=fontList.first(); fontp != 0; fontp=fontList.next() ) 
    if ((fontp->flags & font::FONT_KPSE_NAME) == 0) { 
#ifdef DEBUG_FONTPOOL
      kdDebug() << "... no, there are still fonts missing." << endl;
#endif

      // Mark that we already tried to find the font name, so that we do
      // not try again, even if that fails.
      fontp->flags |= font::FONT_KPSE_NAME; 
   
      // Just make sure that MetafontMode is in the permissible range, so
      // as to avoid core dumps.
      if (MetafontMode >= NumberOfMFModes) {
	kdError() << "fontPool::appendx called with bad MetafontMode " << MetafontMode 
		  << " which is more than the allowed value of " << NumberOfMFModes-1 << endl;
	kdError() << "setting mode to " << MFModes[DefaultMFMode] << " at " 
		  << MFResolutions[DefaultMFMode] << "dpi" << endl;
	MetafontMode = DefaultMFMode;
      }

      // We attach the ShellProcess to the font. That way the font will be
      // informed when the file name has been found.
      proc = new KShellProcess();
      if (proc == 0) {
	kdError() << "Could not allocate ShellProcess for the kpsewhich command." << endl;
	exit(0);
      }
      qApp->connect(proc, SIGNAL(receivedStdout(KProcess *, char *, int)),
		    fontp, SLOT(font_name_receiver(KProcess *, char *, int)));
      
      // First try if the font is a virtual font
      proc->clearArguments();
      *proc << "kpsewhich";
      *proc << "--format vf";
      *proc << fontp->fontname;
      proc->start(KProcess::NotifyOnExit, KProcess::All);
      while(proc->isRunning())
	qApp->processEvents();
      
      // Font not found? Then check if the font is a regular font.
      if (fontp->filename.isEmpty()) {
	MetafontOutput.left(0);
	qApp->connect(proc, SIGNAL(processExited(KProcess *)),
		      this, SLOT(kpsewhich_terminated(KProcess *)));
	qApp->connect(proc, SIGNAL(receivedStderr(KProcess *, char *, int)),
		      this, SLOT(mf_output_receiver(KProcess *, char *, int)));

	proc->clearArguments();
	*proc << "kpsewhich";
	*proc << QString("--dpi %1").arg(MFResolutions[MetafontMode]);
	*proc << QString("--mode %1").arg(MFModes[MetafontMode]);
	*proc << "--format pk";
	if (makepk == 0)
	  *proc << "--no-mktex pk";
	else
	  *proc << "--mktex pk";
	*proc << QString("%1.%2").arg(fontp->fontname).arg((int)(fontp->fsize + 0.5));

#ifdef DEBUG_FONTPOOL	
	kdDebug() << "kpsewhich"
		  << QString(" --dpi %1").arg(MFResolutions[MetafontMode])
		  << QString(" --mode %1").arg(MFModes[MetafontMode])
		  << " --format pk"
		  << QString(" %1.%2").arg(fontp->fontname).arg((int)(fontp->fsize + 0.5))
		  << endl;
#endif

	proc->start(KProcess::NotifyOnExit, KProcess::All);
	return -1;
      }

      delete proc;
      proc = 0;

      check_if_fonts_are_loaded();

      return -1; // That says that not all fonts are loaded.
    }

#ifdef DEBUG_FONTPOOL
  kdDebug() << "... yes, all fonts are there, or could not be found." << endl;
#endif

  return 0; // That says that all fonts are loaded.
}


void fontPool::kpsewhich_terminated(KProcess *)
{
#ifdef DEBUG_FONTPOOL
  kdDebug() << "kpsewhich terminated" << endl;
#endif

  delete proc;
  proc = 0;

  if (check_if_fonts_are_loaded() == 0) {
#ifdef DEBUG_FONTPOOL
    kdDebug() << "Emitting fonts_have_been_loaded()" << endl;
#endif
    emit(fonts_have_been_loaded());
  }
  return;
}


void fontPool::reset_fonts(void)
{
#ifdef DEBUG_FONTPOOL
  kdDebug() << "Reset Fonts" << endl;
#endif

  struct font  *fontp;
  struct glyph *glyphp;

  for ( fontp =fontList.first(); fontp != 0; fontp=fontList.next() )
    if ((fontp->flags & font::FONT_LOADED) && !(fontp->flags & font::FONT_VIRTUAL))
      for (glyphp = fontp->glyphtable; glyphp < fontp->glyphtable + font::max_num_of_chars_in_font; ++glyphp)
	glyphp->clearShrunkCharacter();
}

void fontPool::mark_fonts_as_unused(void)
{
#ifdef DEBUG_FONTPOOL
  kdDebug() << "Mark_fonts_as_unused" << endl;
#endif

  struct font  *fontp;

  for ( fontp =fontList.first(); fontp != 0; fontp=fontList.next() )
    fontp->flags &= ~font::FONT_IN_USE; 
}

void fontPool::release_fonts(void)
{
#ifdef DEBUG_FONTPOOL
  kdDebug() << "Release_fonts" << endl;
#endif
  
  struct font  *fontp = fontList.first();

  while(fontp != 0) {
    if ((fontp->flags & font::FONT_IN_USE) != font::FONT_IN_USE) {
      fontList.removeRef(fontp);
      fontp = fontList.first();
    } else 
      fontp = fontList.next();
  }
}

void fontPool::mf_output_receiver(KProcess *, char *buffer, int buflen)
{
  int numleft;

  // Paranoia.
  if (buflen < 0)
    return;

  MetafontOutput.append(QString::fromLocal8Bit(buffer, buflen));

  // We'd like to print only full lines of text.
  while( (numleft = MetafontOutput.find('\n')) != -1) {
    kdDebug() << "MF OUTPUT RECEIVED: " << MetafontOutput.left(numleft+1);
    MetafontOutput = MetafontOutput.remove(0,numleft+1);
  }
}
