// fontpool.cpp
//
// (C) 2001-2003 Stefan Kebekus
// Distributed under the GPL

#include <kconfig.h>
#include <kdebug.h>
#include <kinstance.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kprocio.h>
#include <kstringhandler.h>

#include <qapplication.h>
#include <qfile.h>

#include <stdlib.h>

#include "fontpool.h"
#include "fontprogress.h"
#include "performanceMeasurement.h"
#include "TeXFont.h"

// List of permissible MetaFontModes which are supported by kdvi.

const char *MFModes[]       = { "cx", "ljfour", "lexmarks" };
const char *MFModenames[]   = { "Canon CX", "LaserJet 4", "Lexmark S" };
const int   MFResolutions[] = { 300, 600, 1200 };

#ifdef PERFORMANCE_MEASUREMENT
QTime fontPoolTimer;
bool fontPoolTimerFlag;
#endif

//#define DEBUG_FONTPOOL

fontPool::fontPool(void)
{
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "fontPool::fontPool(void) called" << endl;
#endif

  setName("Font Pool");

  proc                     = 0;
  makepk                   = true; // By default, fonts are generated
  displayResolution_in_dpi = 100.0; // A not-too-bad-default
  MetafontMode             = DefaultMFMode;
  fontList.setAutoDelete(TRUE);


#ifdef HAVE_FREETYPE
  // Initialize the Freetype Library
  if ( FT_Init_FreeType( &FreeType_library ) != 0 ) {
    kdError(4300) << "Cannot load the FreeType library. KDVI proceeds without FreeType support." << endl;
    FreeType_could_be_loaded = false;
  } else 
    FreeType_could_be_loaded = true;
#endif

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
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "fontPool::~fontPool(void) called" << endl;
#endif

#ifdef HAVE_FREETYPE
  if (FreeType_could_be_loaded == true)
    FT_Done_FreeType( FreeType_library );
#endif

  if (progress)
    delete progress;
}


void fontPool::setParameters( unsigned int _metafontMode, bool _makePK, bool _useType1Fonts, bool _useFontHints )
{
  if (_metafontMode >= NumberOfMFModes) {
    kdError(4300) << "fontPool::setMetafontMode called with argument " << _metafontMode 
		  << " which is more than the allowed value of " << NumberOfMFModes-1 << endl;
    kdError(4300) << "setting mode to " << MFModes[DefaultMFMode] << " at " 
		  << MFResolutions[DefaultMFMode] << "dpi" << endl;
    _metafontMode = DefaultMFMode;
  }
  
  bool kpsewhichNeeded = false;
  
  // Check if a new run of kpsewhich is required
  if ( (_metafontMode != MetafontMode) || (_useType1Fonts != useType1Fonts) ) {
    TeXFontDefinition *fontp = fontList.first();
    while(fontp != 0 ) {
      fontp->reset();
      fontp = fontList.next();
    }
    kpsewhichNeeded = true;
  }
  
  // If we enable font generation, we look for fonts which have not
  // yet been loaded, mark them as "not yet looked up" and try to run
  // kpsewhich once more.
  if ((_makePK == true) && (_makePK != makepk)) {
    TeXFontDefinition *fontp =fontList.first();
    while(fontp != 0 ) {
      if (fontp->filename.isEmpty() )
	fontp->flags &= ~TeXFontDefinition::FONT_KPSE_NAME;
      fontp=fontList.next();
    }
    kpsewhichNeeded = true;
  }
  
  // Check if glyphs need to be cleared
  if (_useFontHints != useFontHints) {
    double displayResolution = displayResolution_in_dpi;
    TeXFontDefinition *fontp = fontList.first();
    while(fontp != 0 ) {
      fontp->setDisplayResolution(displayResolution * fontp->enlargement);
      fontp=fontList.next();
    }
  }

  MetafontMode = _metafontMode;
  makepk = _makePK;
  useType1Fonts = _useType1Fonts;
  useFontHints = _useFontHints;
  
  // Initiate a new concurrently running process of kpsewhich, if
  // necessary. Otherwise, let the dvi window be redrawn
  if (kpsewhichNeeded == true)
    check_if_fonts_filenames_are_looked_up();
  else
    emit fonts_have_been_loaded(this);
}


class TeXFontDefinition *fontPool::appendx(QString fontname, Q_UINT32 checksum, Q_UINT32 scale, double enlargement)
{
  // Reuse font if possible: check if a font with that name and
  // natural resolution is already in the fontpool, and use that, if
  // possible.
  TeXFontDefinition *fontp = fontList.first();
  while( fontp != 0 ) {
    if ((fontname == fontp->fontname) && ( (int)(enlargement*1000.0+0.5)) == (int)(fontp->enlargement*1000.0+0.5)) {
      // if font is already in the list
      fontp->mark_as_used();
      return fontp;
    }
    fontp=fontList.next();
  }
  
  // If font doesn't exist yet, we have to generate a new font.
  
  double displayResolution = displayResolution_in_dpi;

  fontp = new TeXFontDefinition(fontname, displayResolution*enlargement, checksum, scale, this, enlargement);
  if (fontp == 0) {
    kdError(4300) << i18n("Could not allocate memory for a font structure!") << endl;
    exit(0);
  }
  fontList.append(fontp);
 
#ifdef PERFORMANCE_MEASUREMENT
  fontPoolTimer.start();
  fontPoolTimerFlag = false;
#endif
 
  // Now start kpsewhich/MetaFont, etc. if necessary
  return fontp;
}


QString fontPool::status(void)
{
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "fontPool::status() called" << endl;
#endif

  QString       text;
  QStringList   tmp;

  if (fontList.isEmpty()) 
    return i18n("The fontlist is currently empty.");

  text.append("<table WIDTH=\"100%\" NOSAVE >");
  text.append( QString("<tr><td><b>%1</b></td> <td><b>%2</b></td> <td><b>%3</b></td> <td><b>%4</b></td> <td><b>%5</b></td></tr>").arg(i18n("Name")).arg(i18n("Enlargement")).arg(i18n("Type")).arg(i18n("Filename")).arg(i18n("Comment")) );
  
 TeXFontDefinition *fontp = fontList.first();
  while ( fontp != 0 ) {
    QString type;
    
    if (fontp->flags & TeXFontDefinition::FONT_VIRTUAL)
      type = i18n("virtual");
    else
      type = i18n("regular");
    
    if (fontp->font == 0)
      tmp << QString ("<tr><td>%1</td> <td>%2%</td> <td>%3</td> <td>%4</td> <td>%5</td></tr>").arg(fontp->fontname).arg((int)(fontp->enlargement*100 + 0.5)).arg(type).arg(fontp->filename).arg("");
    else
      tmp << QString ("<tr><td>%1</td> <td>%2%</td> <td>%3</td> <td>%4</td> <td>%5</td></tr>").arg(fontp->fontname).arg((int)(fontp->enlargement*100 + 0.5)).arg(type).arg(fontp->filename).arg(fontp->font->errorMessage);
    fontp=fontList.next(); 
  }

  tmp.sort();
  text.append(tmp.join("\n"));

  text.append("</table>");

  return text;
}


bool fontPool::check_if_fonts_filenames_are_looked_up(void) 
{
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "fontPool::check_if_fonts_filenames_are_looked_up(void) called" << endl;
#endif
  
  // Check if kpsewhich is still running. In that case certainly not
  // all fonts have been properly looked up.
  if (proc != 0) {
#ifdef DEBUG_FONTPOOL
    kdDebug(4300) << "... no, kpsewhich is still running." << endl;
#endif
    return false;
  }

  // Is there a font whose name we did not try to find out yet?
  TeXFontDefinition *fontp = fontList.first();
  while( fontp != 0 ) {
    if ((fontp->flags & TeXFontDefinition::FONT_KPSE_NAME) == 0)
      break;
    fontp=fontList.next();
  }
  
  if (fontp == 0) {
#ifdef DEBUG_FONTPOOL
    kdDebug(4300) << "... yes, all fonts are there, or could not be found." << endl;
#endif
    return true; // That says that all fonts are loaded.
  }

  pass = 0;
  start_kpsewhich();
  return false; // That says that not all fonts are loaded.  
}


void fontPool::start_kpsewhich(void)
{
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "fontPool::start_kpsewhich(void) called" << endl;
#endif

  // Check if kpsewhich is still running. In that case we certainly do
  // not want to run another kpsewhich process
  if (proc != 0) {
#ifdef DEBUG_FONTPOOL
    kdDebug(4300) << "kpsewhich is still running." << endl;
#endif
    return;
  }
   
  // Just make sure that MetafontMode is in the permissible range, so
  // as to avoid segfaults.
  if (MetafontMode >= NumberOfMFModes) {
    kdError(4300) << "fontPool::appendx called with bad MetafontMode " << MetafontMode 
		  << " which is more than the allowed value of " << NumberOfMFModes-1 << endl
		  << "setting mode to " << MFModes[DefaultMFMode] << " at " 
		  << MFResolutions[DefaultMFMode] << "dpi" << endl;
    MetafontMode = DefaultMFMode;
  }
  
  // Set up the kpsewhich process. If pass == 0, look for vf-fonts and
  // disable automatic font generation as vf-fonts can't be
  // generated. If pass == 0, ennable font generation, if it was
  // enabled globally.
  emit setStatusBarText(i18n("Locating fonts..."));
  
#ifdef DEBUG_FONTPOOL
  QString shellProcessCmdLine;
#endif
  
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
#ifdef DEBUG_FONTPOOL
  shellProcessCmdLine += "kpsewhich ";
#endif
  *proc << QString("--dpi %1").arg(MFResolutions[MetafontMode]);
#ifdef DEBUG_FONTPOOL
  shellProcessCmdLine += QString("--dpi %1").arg(MFResolutions[MetafontMode]) + " ";
#endif
  *proc << QString("--mode %1").arg(KShellProcess::quote(MFModes[MetafontMode]));
#ifdef DEBUG_FONTPOOL
  shellProcessCmdLine += QString("--mode %1").arg(KShellProcess::quote(MFModes[MetafontMode])) + " ";
#endif

  // Enable automatic pk-font generation only in the second pass. (If
  // automatic font generation is switched off, this method will never
  // be called with pass==1)
  if (pass == 1) {
    *proc << "--mktex pk";
#ifdef DEBUG_FONTPOOL
    shellProcessCmdLine +=  "--mktex pk ";
#endif
  } else {
    *proc << "--no-mktex pk";
#ifdef DEBUG_FONTPOOL
    shellProcessCmdLine +=  "--no-mktex pk ";
#endif
  }
  
  int numFontsInJob = 0;

  TeXFontDefinition *fontp = fontList.first();
  while ( fontp != 0 ) {
    if ((fontp->flags & TeXFontDefinition::FONT_KPSE_NAME) == 0) {
      numFontsInJob++;

      switch(pass){
      case 0:
	// In the first pass, we look for PK fonts, and also for virtual fonts.
#ifdef HAVE_FREETYPE
	if ((useType1Fonts == true) && (FreeType_could_be_loaded == true)) {
	  const QString &filename = fontsByTeXName.findFileName(fontp->fontname);
	  *proc << KShellProcess::quote(QString("%1").arg(filename));
#ifdef DEBUG_FONTPOOL
	  shellProcessCmdLine += KShellProcess::quote(QString("%1").arg(filename)) + " ";
#endif
	}
#endif
	*proc << KShellProcess::quote(QString("%1.vf").arg(fontp->fontname));
	*proc << KShellProcess::quote(QString("%2.%1pk").arg(MFResolutions[MetafontMode]).arg(fontp->fontname));
#ifdef DEBUG_FONTPOOL
	shellProcessCmdLine += KShellProcess::quote(QString("%1.vf").arg(fontp->fontname)) + " ";
	shellProcessCmdLine += KShellProcess::quote(QString("%2.%1pk").arg(MFResolutions[MetafontMode]).arg(fontp->fontname)) + " ";
#endif
	break;
      case 1:
	// In the second pass, we generate PK fonts, but we also look
	// for PFB fonts, as they might be used by virtual fonts.
#ifdef HAVE_FREETYPE
	if ((useType1Fonts == true) && (FreeType_could_be_loaded == true)) {
	  const QString &filename = fontsByTeXName.findFileName(fontp->fontname);
	  *proc << KShellProcess::quote(QString("%1").arg(filename));
#ifdef DEBUG_FONTPOOL
	  shellProcessCmdLine += KShellProcess::quote(QString("%1").arg(filename)) + " ";
#endif
	}
#endif
	*proc << KShellProcess::quote(QString("%2.%1pk").arg(MFResolutions[MetafontMode]).arg(fontp->fontname));
#ifdef DEBUG_FONTPOOL
	shellProcessCmdLine += KShellProcess::quote(QString("%2.%1pk").arg(MFResolutions[MetafontMode]).arg(fontp->fontname)) + " ";
#endif
	break;
      default:
	// In the third pass, as a last resort, try to find TFM files
	*proc << KShellProcess::quote(QString("%1.tfm").arg(fontp->fontname));
#ifdef DEBUG_FONTPOOL
	shellProcessCmdLine += KShellProcess::quote(QString("%1.tfm").arg(fontp->fontname)) + " ";
#endif
	break;
      }

      // In the second (last) pass, mark the font "looked up". As this
      // is the last chance that the filename could be found, we
      // ensure that if the filename is still not found now, we won't
      // look any further.
      if (pass == 2)
	fontp->flags |= TeXFontDefinition::FONT_KPSE_NAME;
    }
    fontp=fontList.next();
  }
  
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "pass " << pass << " kpsewhich run with " << numFontsInJob << "fonts to locate." << endl;
  kdDebug(4300) << "command line: " << shellProcessCmdLine << endl;
#endif
  
  if (pass != 0)
    emit(totalFontsInJob(numFontsInJob));
  
  kpsewhichOutput = "";
  MetafontOutput  = "";
  proc->closeStdin();
  if (proc->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false) {
    kdError(4300) << "kpsewhich failed to start" << endl;
  }
}


void fontPool::kpsewhich_terminated(KProcess *)
{
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "kpsewhich terminated. Output was " << endl << kpsewhichOutput << endl;
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
    TeXFontDefinition *fontp=fontList.first();
    while ( fontp != 0 ) { 
      fontp->flags |= TeXFontDefinition::FONT_KPSE_NAME;
      fontp = fontList.next();
    }
  }

  delete proc;
  proc = 0;

  QStringList fileNameList = QStringList::split('\n', kpsewhichOutput);

  TeXFontDefinition *fontp=fontList.first();
  while ( fontp != 0 ) { 
    if (fontp->filename.isEmpty() == true) {
      QStringList matchingFiles;
#ifdef HAVE_FREETYPE
      const QString &fn = fontsByTeXName.findFileName(fontp->fontname);
      if (!fn.isEmpty())
	matchingFiles = fileNameList.grep(fn);
#endif
      if (matchingFiles.isEmpty() == true) 
	matchingFiles += fileNameList.grep(fontp->fontname);
      
      if (matchingFiles.isEmpty() != true) {
#ifdef DEBUG_FONTPOOL
	kdDebug(4300) << "Associated " << fontp->fontname << " to " << matchingFiles.first() << endl;
#endif
	fontp->fontNameReceiver(matchingFiles.first());
	fontp->flags |= TeXFontDefinition::FONT_KPSE_NAME;
	// Constructing a virtual font will most likely insert other
	// fonts into the fontList. After that, fontList.next() will
	// no longer work. It is therefore safer to start over.
	fontp=fontList.first();
	continue;
      }
    } // of if (fontp->filename.isEmpty() == true)
    fontp = fontList.next();
  }

  // Check if some font filenames are still missing. If not, or if we
  // have just finished the last pass, we quit here.
  bool all_fonts_are_found = true;
  
  fontp = fontList.first();
  while ( fontp != 0 ) {
    if (fontp->filename.isEmpty() == true) {
      all_fonts_are_found = false;
	break;
    }
    fontp=fontList.next();
  }
  if ((all_fonts_are_found) || (pass >= 2)) {
#ifdef DEBUG_FONTPOOL
    kdDebug(4300) << "Emitting fonts_have_been_loaded(this)" << endl;
#endif
#ifdef PERFORMANCE_MEASUREMENT
    kdDebug(4300) << "Time required to look up fonts: " << fontPoolTimer.elapsed() << "ms" << endl;
#endif
    emit setStatusBarText(QString::null);
    emit fonts_have_been_loaded(this);
    return;
  }
  
  if (pass == 0) {
    pass = 1;
    // If automatic pk-font generation is enabled, we call
    // check_if_fonts_filenames_are_looked_up.
    if (makepk != 0) {
      start_kpsewhich();
      return;
    }
  }
  
  if (pass == 1) {
    // Now all fonts should be there. It may, however, have happened
    // that still not all fonts were found. If that is so, issue a
    // warning here.
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
      QString nokps = i18n("There were problems running the kpsewhich program. "
			   "KDVI will not work if TeX is not installed on your "
			   "system or if the kpsewhich program cannot be found "
			   "in the standard search path.\n");
      QString body  = i18n("KDVI was not able to locate all the font files "
			   "which are necessary to display the current DVI file. "
			   "Your document might be unreadable.");
      QString metaf = i18n("\nExperts will find helpful information in the 'MetaFont'-"
			   "section of the document info dialog");
      
      if (fatal_error_in_kpsewhich == true)
	KMessageBox::sorry( 0, nokps+body+metaf, title );
      else
	if (makepk == 0) {
	  if(KMessageBox::warningYesNo( 0, body+i18n("\nAutomatic font generation is switched off. "
						     "You might want to switch it on now and generate the missing fonts."), title,
					i18n("Generate Fonts Now"), i18n("Continue Without"), "WarnForMissingFonts" ) == KMessageBox::Yes) {
	    KInstance *instance = new KInstance("kdvi");
	    KConfig *config = instance->config();
	    config->setGroup("kdvi");
	    config->writeEntry( "MakePK", true );
	    config->sync();
	    setParameters( MetafontMode, true, useType1Fonts, useFontHints ); // That will start kpsewhich again.
	    return;
	  } 
	} else
	  KMessageBox::sorry( 0, body+metaf, title );
    }
    
    pass = 2;
    start_kpsewhich();
    return;
  }
}


void fontPool::setDisplayResolution( double _displayResolution_in_dpi )
{
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "fontPool::setDisplayResolution( displayResolution_in_dpi=" << _displayResolution_in_dpi << " ) called" << endl;
#endif
  displayResolution_in_dpi = _displayResolution_in_dpi;
  double displayResolution = displayResolution_in_dpi;

  TeXFontDefinition *fontp = fontList.first();
  while(fontp != 0 ) {
    fontp->setDisplayResolution(displayResolution * fontp->enlargement);
    fontp=fontList.next();
  }
  
  // Do something that causes re-rendering of the dvi-window
  emit fonts_have_been_loaded(this);
}


void fontPool::mark_fonts_as_unused(void)
{
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "fontPool::mark_fonts_as_unused(void) called" << endl;
#endif

  TeXFontDefinition  *fontp = fontList.first();
  while ( fontp != 0 ) {
    fontp->flags &= ~TeXFontDefinition::FONT_IN_USE; 
    fontp=fontList.next();
  }
}


void fontPool::release_fonts(void)
{
#ifdef DEBUG_FONTPOOL
  kdDebug(4300) << "Release_fonts" << endl;
#endif
  
  TeXFontDefinition  *fontp = fontList.first();
  while(fontp != 0) {
    if ((fontp->flags & TeXFontDefinition::FONT_IN_USE) != TeXFontDefinition::FONT_IN_USE) {
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
