// fontpool.cpp
//
// (C) 2001 Stefan Kebekus
// Distributed under the GPL

#include <kconfig.h>
#include <kdebug.h>
#include <kinstance.h>
#include <klocale.h>
#include <kprocess.h>
#include <qapplication.h>
#include <qmessagebox.h>
#include <stdlib.h>

#include "font.h"
#include "fontpool.h"
#include "fontprogress.h"
#include "kdvi.h"
#include "xdvi.h"

// List of permissible MetaFontModes which are supported by kdvi.

const char *MFModes[]       = { "cx", "ljfour", "lexmarks" };
const char *MFModenames[]   = { "Canon CX", "LaserJet 4", "Lexmark S" };
const int   MFResolutions[] = { 300, 600, 1200 };
 

fontPool::fontPool(void)
{
  setName("Font Pool");

  proc = 0;
  fontList.setAutoDelete(TRUE);

  progress = new fontProgressDialog( "fontgen",  // Chapter in the documentation for help.
				     i18n( "KDVI is currently generating bitmap fonts..." ),
				     i18n( "Aborts the font generation. Don't do this." ),
				     i18n( "KDVI is currently generating bitmap fonts which are needed to display your document. "
					   "For this, KDVI uses a number of external programs, such as MetaFont. You can find "
					   "the output of these programs later in the document info dialog." ),
				     i18n( "KDVI is generating fonts. Please wait." ),
				     0 );
  if (progress == NULL)
    kdError(4300) << "Could not allocate memory for the font progress dialog." << endl;
  else {
    qApp->connect(this, SIGNAL(hide_progress_dialog()), progress, SLOT(hideDialog()));
    qApp->connect(this, SIGNAL(totalFontsInJob(int)), progress, SLOT(setTotalSteps(int)));
    qApp->connect(this, SIGNAL(show_progress(void)), progress, SLOT(show(void)));
    qApp->connect(progress, SIGNAL(finished(void)), this, SLOT(abortGeneration(void)));
  }
}

fontPool::~fontPool(void)
{
  if (progress)
    delete progress;
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

  // If we just disabled font generation, there is nothing left to do.
  if (flag == 0)
    return;

  // If we enable font generation, we look for fonts which have not
  // yet been loaded, mark them as "not yet looked up" and try once
  // more.
  struct font *fontp = fontp=fontList.first();
  while(fontp != 0 ) {
    if (fontp->filename.isEmpty() )
      fontp->flags &= ~font::FONT_KPSE_NAME;
    fontp=fontList.next();
  }
  check_if_fonts_are_loaded();
}


class font *fontPool::appendx(char *fontname, long checksum, int scale, int design, float fsize, double scale_dimconv)
{
  // Reuse font if possible: check if a font with that name and fsize
  // is already in the fontpool, and use that, if possible.
  class font *fontp = fontList.first();
  while( fontp != 0 ) {
    if (strcmp(fontname, fontp->fontname) == 0 && (int (fsize+0.5)) == (int)(fontp->fsize + 0.5)) {
      // if font is already in the list
      fontp->mark_as_used();
      free(fontname);
      return fontp;
    }
    fontp=fontList.next();
  }

  // If font doesn't exist yet, we have to generate a new font.
  fontp = new font(fontname, fsize, checksum, scale*scale_dimconv/(1<<20), this);
  if (fontp == 0) {
    kdError(4300) << i18n("Could not allocate memory for a font structure!") << endl;
    exit(0);
  }
  fontList.append(fontp);

  // Now start kpsewhich/MetaFont, etc. if necessary
  return fontp;
}


QString fontPool::status(void)
{
  QString       text;
  QStringList   tmp;

  if (fontList.isEmpty()) 
    return i18n("The fontlist is currently empty.");

  text.append("<table WIDTH=\"100%\" NOSAVE >");
  text.append("<tr><td><b>Name</b></td> <td><b>DPI</b></td> <td><b>Type</b></td> <td><b>Filename</b></td></tr>");
 
  struct font  *fontp = fontList.first();
  while ( fontp != 0 ) {
    QString type;

    if (fontp->flags & font::FONT_VIRTUAL)
      type = i18n("virtual");
    else
      type = i18n("regular");

    tmp << QString ("<tr><td>%1</td> <td>%2</td> <td>%3</td> <td>%4</td></tr>").arg(fontp->fontname).arg((int)(fontp->fsize+0.5)).arg(type).arg(fontp->filename);
    fontp=fontList.next(); 
 }

  tmp.sort();
  text.append(tmp.join("\n"));

  text.append("</table>");

  return text;
}


char fontPool::check_if_fonts_are_loaded(unsigned char pass) 
{
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "Check if fonts have been looked for..." << endl;
#endif

  // Check if kpsewhich is still running. In that case certainly not
  // all fonts have been properly looked up.
  if (proc != 0) {
#ifdef DEBUG_FONTPOOL
    kdDebug(4300) << "... no, kpsewhich is still running." << endl;
#endif
    emit fonts_info(this);
    return -1;
  }

  // Is there a font whose name we did not try to find out yet?
  struct font *fontp = fontList.first();
  while( fontp != 0 ) {
    if ((fontp->flags & font::FONT_KPSE_NAME) == 0)
      break;
    fontp=fontList.next();
  }

  if (fontp == 0) {
#ifdef DEBUG_FONTPOOL
    kdDebug(4300) << "... yes, all fonts are there, or could not be found." << endl;
#endif
    emit fonts_info(this);
    return 0; // That says that all fonts are loaded.
  }
   
  // Just make sure that MetafontMode is in the permissible range, so
  // as to avoid core dumps.
  if (MetafontMode >= NumberOfMFModes) {
    kdError(4300) << "fontPool::appendx called with bad MetafontMode " << MetafontMode 
		  << " which is more than the allowed value of " << NumberOfMFModes-1 << endl
		  << "setting mode to " << MFModes[DefaultMFMode] << " at " 
		  << MFResolutions[DefaultMFMode] << "dpi" << endl;
    MetafontMode = DefaultMFMode;
  }

  // Set up the kpsewhich process. If pass == 0, look for vf-fonts and
  // disable automatic font generation as vf-fonts can't be
  // generated. If pass != 0, ennable font generation, if it was
  // enabled globally.
  emit setStatusBarText(i18n("Locating fonts..."));
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
  emit(new_kpsewhich_run(i18n("Font Generation")));

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
  int numFontsInJob = 0;

  fontp = fontList.first();;
  while ( fontp != 0 ) {
    if ((fontp->flags & font::FONT_KPSE_NAME) == 0) {
      numFontsInJob++;
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
    fontp=fontList.next();
  }

  if (pass != 0)
    emit(totalFontsInJob(numFontsInJob));

  kpsewhichOutput = "";
  MetafontOutput  = "";
  proc->closeStdin();
  if (proc->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false) {
    kdError(4300) << "kpsewhich failed to start" << endl;
  }

  emit fonts_info(this);
  return -1; // That says that not all fonts are loaded.
}


void fontPool::kpsewhich_terminated(KProcess *)
{
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "kpsewhich terminated" << endl;
#endif

  emit(hide_progress_dialog());

  // Handle fatal errors. The content of fatal_error_in_kpsewhich will
  // later be used to generate the proper text for error messages.
  bool fatal_error_in_kpsewhich = false;
  if (proc->normalExit())
    if (proc->exitStatus() == 127)
      fatal_error_in_kpsewhich = true;
  if (fatal_error_in_kpsewhich) {
    // Mark all fonts as done so kpsewhich will not be called again
    // soon.
    class font *fontp=fontList.first();
    while ( fontp != 0 ) { 
      fontp->flags |= font::FONT_KPSE_NAME;
      fontp = fontList.next();
    }
  }

   delete proc;
  proc = 0;

  QStringList fileNameList = QStringList::split('\n', kpsewhichOutput);

  class font *fontp=fontList.first();
  while ( fontp != 0 ) { 
    if (fontp->filename.isEmpty() == true) {
      QString fontname = QString("%1.%2pk").arg(fontp->fontname).arg((int)(fontp->fsize + 0.5));
      QStringList matchingFiles = fileNameList.grep(fontname);
      if (matchingFiles.isEmpty() != true) {
#ifdef DEBUG_FONTPOOL
	kdDebug(4300) << "Associated " << fontname << " to " << matchingFiles.first() << endl;
#endif
	fontp->fontNameReceiver(matchingFiles.first());
	fontp->flags |= font::FONT_KPSE_NAME;
	fontp=fontList.first();
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
	emit fonts_info(this);
	// Constructing a virtual font will most likely insert other
	// fonts into the fontList. After that, fontList.next() will
	// no longer work. It is therefore safer to start over.
	fontp=fontList.first();
	continue;
      }
    } // of if (fontp->filename.isEmpty() == true)
    fontp = fontList.next();
  }
  
  if (check_if_fonts_are_loaded(1) == 0) {
#ifdef DEBUG_FONTPOOL
    kdDebug(4300) << "Emitting fonts_have_been_loaded()" << endl;
#endif

    // Now we have at least tried to look up all fonts. It may,
    // however, have happened that still not all fonts were found. If
    // that is so, issue a warning here.
    bool all_fonts_are_found = true;

    fontp = fontList.first();
    while ( fontp != 0 ) {
      if (fontp->filename.isEmpty() == true) {
	all_fonts_are_found = false;
	break;
      }
      fontp=fontList.next();
    }

    if (all_fonts_are_found == false) {
      QString title = i18n("Font not found - KDVI");
      QString nokps = i18n("There were problems running the kpsewhich program.\n"
			   "KDVI will not work if TeX is not installed on your\n"
			   "system or if the kpsewhich program cannot be found\n"
			   "in the standard search path.\n\n");
      QString body  = i18n("KDVI was not able to locate all the font files \n"
			   "which are necessary to display the current DVI file. \n"
			   "Some characters are therefore left blank, and your \n"
			   "document might be unreadable.");
      QString metaf = i18n("\n\nExperts will find helpful information in the 'MetaFont'-\n"
			   "section of the document info dialog");

      if (fatal_error_in_kpsewhich == true)
	QMessageBox::warning( 0, title, nokps+body+metaf );
      else
	if (makepk == 0) {
	  if(QMessageBox::warning( 0, title, body+i18n("\nAutomatic font generation is switched off.\n"),
				   "Generate fonts now", "Continue without" ) == 0) {
	    KInstance *instance = new KInstance("kdvi");
	    KConfig *config = instance->config();
	    config->setGroup("kdvi");
	    config->writeEntry( "MakePK", true );
	    config->sync();
	    setMakePK(1);
	    return;
	  } 
	} else
	  QMessageBox::warning( 0, title, body+metaf );
    }
    
    emit(fonts_have_been_loaded());
  }
  return;
}


void fontPool::reset_fonts(void)
{
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "Reset Fonts" << endl;
#endif

  struct font  *fontp = fontList.first();
  while ( fontp != 0 ) {
    if ((fontp->flags & font::FONT_LOADED) && !(fontp->flags & font::FONT_VIRTUAL)) {
      struct glyph *glyphp;
      for (glyphp = fontp->glyphtable; glyphp < fontp->glyphtable + font::max_num_of_chars_in_font; ++glyphp)
	glyphp->clearShrunkCharacter();
    }
    fontp=fontList.next();
  }
}

void fontPool::mark_fonts_as_unused(void)
{
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "Mark_fonts_as_unused" << endl;
#endif

  struct font  *fontp = fontList.first();
  while ( fontp != 0 ) {
    fontp->flags &= ~font::FONT_IN_USE; 
    fontp=fontList.next();
  }
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
  // Paranoia.
  if (buflen < 0)
    return;

  QString op = QString::fromLocal8Bit(buffer, buflen);

  MetafontOutput.append(op);

  // We'd like to print only full lines of text.
  int numleft;
  bool show_prog = false;
  while( (numleft = MetafontOutput.find('\n')) != -1) {
    QString line = MetafontOutput.left(numleft+1); 
    emit(MFOutput(line));
#ifdef DEBUG_FONTPOOL
    kdDebug(4300) << "MF OUTPUT RECEIVED: " << line;
#endif
    // Search for a line which marks the beginning of a MetaFont run
    // and show the progress dialog at the end of this method.
    if (line.find("kpathsea:") == 0)
      show_prog = true;

    // If the Output of the kpsewhich program contains a line starting
    // with "kpathsea:", this means that a new MetaFont-run has been
    // started. We filter these lines out and update the display
    // accordingly.
    int startlineindex = line.find("kpathsea:");
    if (startlineindex != -1) {
      int endstartline  = line.find("\n",startlineindex);
      QString startLine = line.mid(startlineindex,endstartline-startlineindex);

      // The last word in the startline is the name of the font which we
      // are generating. The second-to-last word is the resolution in
      // dots per inch. Display this info in the text label below the
      // progress bar.
      int lastblank     = startLine.findRev(' ');
      QString fontName  = startLine.mid(lastblank+1);
      int secondblank   = startLine.findRev(' ',lastblank-1);
      QString dpi       = startLine.mid(secondblank+1,lastblank-secondblank-1);

      progress->increaseNumSteps( i18n("Currently generating %1 at %2 dpi").arg(fontName).arg(dpi) );
    }
    
    MetafontOutput = MetafontOutput.remove(0,numleft+1);
  }

  // It is very important that this signal is emitted at the very end
  // of the method. The info-dialog is modal and QT branches into a
  // new event loop. The emit-command will therefore NOT return before
  // the dialog is closed, and the strings will be screwed up.
  if (show_prog == true)
    emit(show_progress());
}


void fontPool::kpsewhich_output_receiver(KProcess *, char *buffer, int buflen)
{
  kpsewhichOutput.append(QString::fromLocal8Bit(buffer, buflen));
  emit(numFoundFonts(kpsewhichOutput.contains('\n')));
}


void fontPool::abortGeneration(void)
{
  kdDebug(4300) << "Font generation is aborted." << endl;
  if (proc != 0) 
    if (proc->isRunning()) {
      proc->kill();
    }
}
#include "fontpool.moc"
