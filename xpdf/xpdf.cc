//========================================================================
//
// xpdf.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#include <aconf.h>
#include "gtypes.h"
#include "GString.h"
#include "parseargs.h"
#include "gfile.h"
#include "gmem.h"
#include "GlobalParams.h"
#include "Object.h"
#include "XPDFApp.h"
#include "config.h"

//------------------------------------------------------------------------
// command line options
//------------------------------------------------------------------------

static char t1libControlStr[16] = "";
static char freetypeControlStr[16] = "";
static char psFileArg[256];
static char paperSize[15] = "";
static int paperWidth = 0;
static int paperHeight = 0;
static GBool level1 = gFalse;
static char textEncName[128] = "";
static char textEOL[16] = "";
static char ownerPassword[33] = "";
static char userPassword[33] = "";
static GBool fullScreen = gFalse;
static char remoteName[100] = "xpdf_";
static GBool doRemoteReload = gFalse;
static GBool doRemoteRaise = gFalse;
static GBool doRemoteQuit = gFalse;
static GBool printCommands = gFalse;
static GBool quiet = gFalse;
static char cfgFileName[256] = "";
static GBool printVersion = gFalse;
static GBool printHelp = gFalse;

static ArgDesc argDesc[] = {
  {"-g",          argStringDummy, NULL,           0,
   "initial window geometry"},
  {"-geometry",   argStringDummy, NULL,           0,
   "initial window geometry"},
  {"-title",      argStringDummy, NULL,           0,
   "window title"},
  {"-cmap",       argFlagDummy,   NULL,           0,
   "install a private colormap"},
  {"-rgb",        argIntDummy,    NULL,           0,
   "biggest RGB cube to allocate (default is 5)"},
  {"-rv",         argFlagDummy,   NULL,           0,
   "reverse video"},
  {"-papercolor", argStringDummy, NULL,           0,
   "color of paper background"},
  {"-z",          argStringDummy, NULL,           0,
   "initial zoom level (-5..5, page, width)"},
#if HAVE_T1LIB_H
  {"-t1lib",      argString,      t1libControlStr, sizeof(t1libControlStr),
   "t1lib font rasterizer control: none, plain, low, high"},
#endif
#if HAVE_FREETYPE_FREETYPE_H | HAVE_FREETYPE_H
  {"-freetype",   argString,      freetypeControlStr, sizeof(freetypeControlStr),
   "FreeType font rasterizer control: none, plain, low, high"},
#endif
  {"-ps",         argString,      psFileArg,      sizeof(psFileArg),
   "default PostScript file name or command"},
  {"-paper",      argString,      paperSize,      sizeof(paperSize),
   "paper size (letter, legal, A4, A3, match)"},
  {"-paperw",     argInt,         &paperWidth,    0,
   "paper width, in points"},
  {"-paperh",     argInt,         &paperHeight,   0,
   "paper height, in points"},
  {"-level1",     argFlag,        &level1,        0,
   "generate Level 1 PostScript"},
  {"-enc",    argString,   textEncName,    sizeof(textEncName),
   "output text encoding name"},
  {"-eol",    argString,   textEOL,        sizeof(textEOL),
   "output end-of-line convention (unix, dos, or mac)"},
  {"-opw",        argString,      ownerPassword,  sizeof(ownerPassword),
   "owner password (for encrypted files)"},
  {"-upw",        argString,      userPassword,   sizeof(userPassword),
   "user password (for encrypted files)"},
  {"-fullscreen", argFlag,        &fullScreen,    0,
   "run in full-screen (presentation) mode"},
  {"-remote",     argString,      remoteName + 5, sizeof(remoteName) - 5,
   "start/contact xpdf remote server with specified name"},
  {"-reload",     argFlag,        &doRemoteReload, 0,
   "reload xpdf remove server window (with -remote only)"},
  {"-raise",      argFlag,        &doRemoteRaise, 0,
   "raise xpdf remote server window (with -remote only)"},
  {"-quit",       argFlag,        &doRemoteQuit,  0,
   "kill xpdf remote server (with -remote only)"},
  {"-cmd",        argFlag,        &printCommands, 0,
   "print commands as they're executed"},
  {"-q",          argFlag,        &quiet,         0,
   "don't print any messages or errors"},
  {"-cfg",        argString,      cfgFileName,    sizeof(cfgFileName),
   "configuration file to use in place of .xpdfrc"},
  {"-v",          argFlag,        &printVersion,  0,
   "print copyright and version info"},
  {"-h",          argFlag,        &printHelp,     0,
   "print usage information"},
  {"-help",       argFlag,        &printHelp,     0,
   "print usage information"},
  {"--help",  argFlag,     &printHelp,     0,
   "print usage information"},
  {"-?",      argFlag,     &printHelp,     0,
   "print usage information"},
  {NULL}
};

//------------------------------------------------------------------------

int main(int argc, char *argv[]) {
  XPDFApp *app;
  GString *fileName;
  int pg;
  GString *destName;
  GString *userPasswordStr, *ownerPasswordStr;
  GBool ok;
  int exitCode;

  exitCode = 0;
  userPasswordStr = ownerPasswordStr = NULL;

  // parse args
  ok = parseArgs(argDesc, &argc, argv);

  // read config file
  globalParams = new GlobalParams(cfgFileName);
  if (psFileArg[0]) {
    globalParams->setPSFile(psFileArg);
  }
  if (paperSize[0]) {
    if (!globalParams->setPSPaperSize(paperSize)) {
      fprintf(stderr, "Invalid paper size\n");
    }
  } else {
    if (paperWidth) {
      globalParams->setPSPaperWidth(paperWidth);
    }
    if (paperHeight) {
      globalParams->setPSPaperHeight(paperHeight);
    }
  }
  if (level1) {
    globalParams->setPSLevel(psLevel1);
  }
  if (textEncName[0]) {
    globalParams->setTextEncoding(textEncName);
  }
  if (textEOL[0]) {
    if (!globalParams->setTextEOL(textEOL)) {
      fprintf(stderr, "Bad '-eol' value on command line\n");
    }
  }
  if (t1libControlStr[0]) {
    if (!globalParams->setT1libControl(t1libControlStr)) {
      fprintf(stderr, "Bad '-t1lib' value on command line\n");
    }
  }
  if (freetypeControlStr[0]) {
    if (!globalParams->setFreeTypeControl(freetypeControlStr)) {
      fprintf(stderr, "Bad '-freetype' value on command line\n");
    }
  }
  if (printCommands) {
    globalParams->setPrintCommands(printCommands);
  }
  if (quiet) {
    globalParams->setErrQuiet(quiet);
  }

  // create the XPDFApp object
  app = new XPDFApp(&argc, argv);

  // the initialZoom parameter can be set in either the config file or
  // as an X resource (or command line arg)
  if (app->getInitialZoom()) {
    globalParams->setInitialZoom(app->getInitialZoom()->getCString());
  }

  // check command line
  ok = ok && argc >= 1 && argc <= 3;
  if (doRemoteReload) {
    ok = ok && remoteName[5] && !doRemoteQuit && argc == 1;
  }
  if (doRemoteRaise) {
    ok = ok && remoteName[5] && !doRemoteQuit;
  }
  if (doRemoteQuit) {
    ok = ok && remoteName[5] && argc == 1;
  }
  if (!ok || printVersion || printHelp) {
    fprintf(stderr, "xpdf version %s\n", xpdfVersion);
    fprintf(stderr, "%s\n", xpdfCopyright);
    if (!printVersion) {
      printUsage("xpdf", "[<PDF-file> [<page> | +<dest>]]", argDesc);
    }
    exitCode = 99;
    goto done1;
  }
  if (argc >= 2) {
    fileName = new GString(argv[1]);
  } else {
    fileName = NULL;
  }
  pg = 1;
  destName = NULL;
  if (argc == 3) {
    if (argv[2][0] == '+') {
      destName = new GString(&argv[2][1]);
    } else {
      pg = atoi(argv[2]);
    }
  }

  // handle remote server stuff
  if (remoteName[5]) {
    app->setRemoteName(remoteName);
    if (app->remoteServerRunning()) {
      if (fileName) {
	if (destName) {
	  app->remoteOpenAtDest(fileName, destName, doRemoteRaise);
	} else {
	  app->remoteOpen(fileName, pg, doRemoteRaise);
	}
      } else if (doRemoteReload) {
	app->remoteReload(doRemoteRaise);
      } else if (doRemoteRaise) {
	app->remoteRaise();
      } else if (doRemoteQuit) {
	app->remoteQuit();
      }
      goto done2;
    }
    if (doRemoteQuit) {
      goto done2;
    }
  }

  // set options
  app->setFullScreen(fullScreen);

  // check for password string(s)
  ownerPasswordStr = ownerPassword[0] ? new GString(ownerPassword)
                                      : (GString *)NULL;
  userPasswordStr = userPassword[0] ? new GString(userPassword)
                                    : (GString *)NULL;

  // open the file and run the main loop
  if (destName) {
    if (!app->openAtDest(fileName, destName,
			 ownerPasswordStr, userPasswordStr)) {
      exitCode = 1;
      goto done2;
    }
  } else {
    if (!app->open(fileName, pg, ownerPasswordStr, userPasswordStr)) {
      exitCode = 1;
      goto done2;
    }
  }
  app->run();

  exitCode = 0;

  // clean up
 done2:
  if (userPasswordStr) {
    delete userPasswordStr;
  }
  if (ownerPasswordStr) {
    delete ownerPasswordStr;
  }
  if (destName) {
    delete destName;
  }
  if (fileName) {
    delete fileName;
  }
 done1:
  delete app;
  delete globalParams;

  // check for memory leaks
  Object::memCheck(stderr);
  gMemReport(stderr);

  return exitCode;
}
