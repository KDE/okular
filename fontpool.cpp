
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
    kdError(4300) << "fontPool::setMetafontMode called with argument " << mode 
	      << " which is more than the allowed value of " << NumberOfMFModes-1 << endl;
    kdError(4300) << "setting mode to " << MFModes[DefaultMFMode] << " at " 
	      << MFResolutions[DefaultMFMode] << "dpi" << endl;
    mode = DefaultMFMode;
  }
  MetafontMode = mode;
  return mode;
}

void fontPool::setMakePK(int flag)
{
  makepk = flag;

  // If we just diabled font generation, there is nothing left to do.
  if (flag == 0)
    return;

  // If we enable font generation, we look for fonts which have not
  // yet been loaded, mark them as "not yet looked up" and try once
  // more.
  struct font *fontp;
  for (fontp=fontList.first(); fontp != 0; fontp=fontList.next() ) 
    if (fontp->filename.isEmpty() )
      fontp->flags &= ~font::FONT_KPSE_NAME;
  check_if_fonts_are_loaded();
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
    kdError(4300) << i18n("Could not allocate memory for a font structure!") << endl;
    exit(0);
  }

  fontList.append(fontp);

  // Now start kpsewhich/MetaFont, etc. if necessary
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
    kdDebug(4300) << entry << endl;
#endif
  }
}


char fontPool::check_if_fonts_are_loaded(unsigned char pass) 
{
  struct font *fontp;

#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "Check if fonts have been looked for..." << endl;
#endif

  // Check if kpsewhich is still running. In that case there certainly
  // not all fonts have been properly looked up.
  if (proc != 0) {
#ifdef DEBUG_FONTPOOL
    kdDebug(4300) << "... no, kpsewhich is still running." << endl;
#endif
    return -1;
  }

  // Is there a font whose name we did not try to find out yet?
  for (fontp=fontList.first(); fontp != 0; fontp=fontList.next() ) 
    if ((fontp->flags & font::FONT_KPSE_NAME) == 0)
      break;
  if (fontp == 0) {
#ifdef DEBUG_FONTPOOL
    kdDebug(4300) << "... yes, all fonts are there, or could not be found." << endl;
#endif
    return 0; // That says that all fonts are loaded.
  }
   
  // Just make sure that MetafontMode is in the permissible range, so
  // as to avoid core dumps.
  if (MetafontMode >= NumberOfMFModes) {
    kdError(4300) << "fontPool::appendx called with bad MetafontMode " << MetafontMode 
		  << " which is more than the allowed value of " << NumberOfMFModes-1 << endl;
    kdError(4300) << "setting mode to " << MFModes[DefaultMFMode] << " at " 
		  << MFResolutions[DefaultMFMode] << "dpi" << endl;
    MetafontMode = DefaultMFMode;
  }

  // Set up the kpsewhich process. If pass == 0, look for vf-fonts and
  // disable automatic font generation as vf-fonts can't be
  // generated. If pass != 0, ennable font generation, if it was
  // enabled globally.
  proc = new KShellProcess();
  if (proc == 0) {
    kdError(4300) << "Could not allocate ShellProcess for the kpsewhich command." << endl;
    exit(0);
  }
  MetafontOutput.left(0);
  qApp->connect(proc, SIGNAL(receivedStdout(KProcess *, char *, int)),
		this, SLOT(kpsewhich_output_receiver(KProcess *, char *, int)));
  qApp->connect(proc, SIGNAL(processExited(KProcess *)),
		this, SLOT(kpsewhich_terminated(KProcess *)));
  qApp->connect(proc, SIGNAL(receivedStderr(KProcess *, char *, int)),
		this, SLOT(mf_output_receiver(KProcess *, char *, int)));
      
  proc->clearArguments();
  *proc << "kpsewhich";
  *proc << QString("--dpi %1").arg(MFResolutions[MetafontMode]);
  *proc << QString("--mode %1").arg(MFModes[MetafontMode]);
  // Enable automatic pk-font generation only in the second pass, and
  // only if the user expressidly asked for it.
  if ((makepk == 0) || (pass == 0))
    *proc << "--no-mktex pk";
  else
    *proc << "--mktex pk";
  for (fontp=fontList.first(); fontp != 0; fontp=fontList.next() ) 
    if ((fontp->flags & font::FONT_KPSE_NAME) == 0) {
      *proc << QString("%1.%2pk").arg(fontp->fontname).arg((int)(fontp->fsize + 0.5));
      // In the first pass, we look also for virtual fonts.
      if (pass == 0)
	*proc << QString("%1.vf").arg(fontp->fontname);
      // In the second (last) pass, mark the font "looked up". As this
      // is the last chance that the filename could be found, we
      // ensure that if the filename is still not found now, we won't
      // look any further.
      if (pass != 0)
	fontp->flags |= font::FONT_KPSE_NAME;
    }

  kpsewhichOutput = "";
  proc->start(KProcess::NotifyOnExit, KProcess::All);
  
  return -1; // That says that not all fonts are loaded.
}


void fontPool::kpsewhich_terminated(KProcess *)
{
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "kpsewhich terminated" << endl;
#endif

  delete proc;
  proc = 0;

  QStringList fileNameList = QStringList::split('\n', kpsewhichOutput);
  for (class font *fontp=fontList.first(); fontp != 0; fontp=fontList.next() ) 
    if (fontp->filename.isEmpty() == true) {
      QString fontname = QString("%1.%2pk").arg(fontp->fontname).arg((int)(fontp->fsize + 0.5));
      QStringList matchingFiles = fileNameList.grep(fontname);
      if (matchingFiles.isEmpty() != true) {
#ifdef DEBUG_FONTPOOL
	kdDebug(4300) << "Associated " << fontname << " to " << matchingFiles.first() << endl;
#endif
	fontp->fontNameReceiver(matchingFiles.first());
	fontp->flags |= font::FONT_KPSE_NAME;
	continue;
      }

      fontname = QString("%1.vf").arg(fontp->fontname);
      matchingFiles = fileNameList.grep(fontname);
      if (matchingFiles.isEmpty() != true) {
#ifdef DEBUG_FONTPOOL
	kdDebug(4300) << "Associated " << fontname << "to " << matchingFiles.first() << endl;
#endif
	fontp->fontNameReceiver(matchingFiles.first());
	fontp->flags |= font::FONT_KPSE_NAME;
	// Constructing a virtual font will most likely insert other
	// fonts into the fontList. After that, fontList.next() will
	// no longer work. It is therefore safer to start over.
	fontp=fontList.first();
      }
    }
  
  if (check_if_fonts_are_loaded(1) == 0) {
#ifdef DEBUG_FONTPOOL
    kdDebug(4300) << "Emitting fonts_have_been_loaded()" << endl;
#endif
    emit(fonts_have_been_loaded());
  }
  return;
}


void fontPool::reset_fonts(void)
{
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "Reset Fonts" << endl;
#endif

  struct font  *fontp;
  struct glyph *glyphp;

  for ( fontp=fontList.first(); fontp != 0; fontp=fontList.next() )
    if ((fontp->flags & font::FONT_LOADED) && !(fontp->flags & font::FONT_VIRTUAL))
      for (glyphp = fontp->glyphtable; glyphp < fontp->glyphtable + font::max_num_of_chars_in_font; ++glyphp)
	glyphp->clearShrunkCharacter();
}

void fontPool::mark_fonts_as_unused(void)
{
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "Mark_fonts_as_unused" << endl;
#endif

  struct font  *fontp;

  for ( fontp =fontList.first(); fontp != 0; fontp=fontList.next() )
    fontp->flags &= ~font::FONT_IN_USE; 
}

void fontPool::release_fonts(void)
{
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "Release_fonts" << endl;
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
    kdDebug(4300) << "MF OUTPUT RECEIVED: " << MetafontOutput.left(numleft+1);
    MetafontOutput = MetafontOutput.remove(0,numleft+1);
  }
}


void fontPool::kpsewhich_output_receiver(KProcess *, char *buffer, int buflen)
{
  kpsewhichOutput.append(QString::fromLocal8Bit(buffer, buflen));
}
