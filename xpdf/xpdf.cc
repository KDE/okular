//========================================================================
//
// xpdf.cc
//
// Copyright 1996-2002 Glyph & Cog, LLC
//
//========================================================================

#include <aconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <X11/X.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include "gtypes.h"
#include "GString.h"
#include "parseargs.h"
#include "gfile.h"
#include "gmem.h"
#include "GlobalParams.h"
#include "LTKAll.h"
#include "Object.h"
#include "Stream.h"
#include "Array.h"
#include "Dict.h"
#include "XRef.h"
#include "Catalog.h"
#include "Page.h"
#include "Link.h"
#include "PDFDoc.h"
#include "XOutputDev.h"
#include "LTKOutputDev.h"
#include "PSOutputDev.h"
#include "TextOutputDev.h"
#include "Error.h"
#include "config.h"

#ifdef XlibSpecificationRelease
#if XlibSpecificationRelease < 5
typedef char *XPointer;
#endif
#else
typedef char *XPointer;
#endif

// hack around old X includes which are missing these symbols
#ifndef XK_Page_Up
#define XK_Page_Up              0xFF55
#endif
#ifndef XK_Page_Down
#define XK_Page_Down            0xFF56
#endif
#ifndef XK_KP_Home
#define XK_KP_Home              0xFF95
#endif
#ifndef XK_KP_Left
#define XK_KP_Left              0xFF96
#endif
#ifndef XK_KP_Up
#define XK_KP_Up                0xFF97
#endif
#ifndef XK_KP_Right
#define XK_KP_Right             0xFF98
#endif
#ifndef XK_KP_Down
#define XK_KP_Down              0xFF99
#endif
#ifndef XK_KP_Prior
#define XK_KP_Prior             0xFF9A
#endif
#ifndef XK_KP_Page_Up
#define XK_KP_Page_Up           0xFF9A
#endif
#ifndef XK_KP_Next
#define XK_KP_Next              0xFF9B
#endif
#ifndef XK_KP_Page_Down
#define XK_KP_Page_Down         0xFF9B
#endif
#ifndef XK_KP_End
#define XK_KP_End               0xFF9C
#endif
#ifndef XK_KP_Begin
#define XK_KP_Begin             0xFF9D
#endif
#ifndef XK_KP_Insert
#define XK_KP_Insert            0xFF9E
#endif
#ifndef XK_KP_Delete
#define XK_KP_Delete            0xFF9F
#endif

//------------------------------------------------------------------------
// misc constants / enums
//------------------------------------------------------------------------

#define remoteCmdLength 512

enum XpdfMenuItem {
  menuOpen,
  menuReload,
  menuSavePDF,
  menuRotateCCW,
  menuRotateCW,
  menuQuit
};

//------------------------------------------------------------------------
// prototypes
//------------------------------------------------------------------------

// loadFile / displayPage
static GBool loadFile(GString *fileName);
static void displayPage(int page1, int zoomA, int rotateA, GBool addToHist);
static void displayDest(LinkDest *dest, int zoomA, int rotateA,
			GBool addToHist);

// key press and menu callbacks
static void keyPressCbk(LTKWindow *win1, KeySym key, Guint modifiers,
			char *s, int n);
static void menuCbk(LTKMenuItem *item);

// mouse callbacks
static void buttonPressCbk(LTKWidget *canvas1, int n,
			   int mx, int my, int button, GBool dblClick);
static void buttonReleaseCbk(LTKWidget *canvas1, int n,
			     int mx, int my, int button, GBool click);
static void doLink(int mx, int my);
static void mouseMoveCbk(LTKWidget *widget, int widgetNum, int mx, int my);
static void mouseDragCbk(LTKWidget *widget, int widgetNum,
			 int mx, int my, int button);

// button callbacks
static void nextPageCbk(LTKWidget *button, int n, GBool on);
static void nextTenPageCbk(LTKWidget *button, int n, GBool on);
static void gotoNextPage(int inc, GBool top);
static void prevPageCbk(LTKWidget *button, int n, GBool on);
static void prevTenPageCbk(LTKWidget *button, int n, GBool on);
static void gotoPrevPage(int dec, GBool top, GBool bottom);
static void backCbk(LTKWidget *button, int n, GBool on);
static void forwardCbk(LTKWidget *button, int n, GBool on);
static void pageNumCbk(LTKWidget *textIn, int n, GString *text);
static void zoomMenuCbk(LTKMenuItem *item);
static void postScriptCbk(LTKWidget *button, int n, GBool on);
static void aboutCbk(LTKWidget *button, int n, GBool on);
static void quitCbk(LTKWidget *button, int n, GBool on);

// scrollbar callbacks
static void scrollVertCbk(LTKWidget *scrollbar, int n, int val);
static void scrollHorizCbk(LTKWidget *scrollbar, int n, int val);

// misc callbacks
static void layoutCbk(LTKWindow *win1);
static void updateScrollbars();
static void propChangeCbk(LTKWindow *win1, Atom atom);

// selection
static void setSelection(int newXMin, int newYMin, int newXMax, int newYMax);

// "Open" dialog
static void mapOpenDialog();
static void openButtonCbk(LTKWidget *button, int n, GBool on);
static void openSelectCbk(LTKWidget *widget, int n, GString *name);

// "Reload"
static void reloadCbk();

// "Save PDF" dialog
static void mapSaveDialog();
static void saveButtonCbk(LTKWidget *button, int n, GBool on);
static void saveSelectCbk(LTKWidget *widget, int n, GString *name);

// "PostScript" dialog
static void mapPSDialog();
static void psButtonCbk(LTKWidget *button, int n, GBool on);

// "About" window
static void mapAboutWin();
static void closeAboutCbk(LTKWidget *button, int n, GBool on);
static void aboutLayoutCbk(LTKWindow *winA);
static void aboutScrollVertCbk(LTKWidget *scrollbar, int n, int val);
static void aboutScrollHorizCbk(LTKWidget *scrollbar, int n, int val);

// "Find" window
static void findCbk(LTKWidget *button, int n, GBool on);
static void mapFindWin();
static void findButtonCbk(LTKWidget *button, int n, GBool on);
static void doFind(char *s);

// app kill callback
static void killCbk(LTKWindow *win1);

//------------------------------------------------------------------------
// "About" window text
//------------------------------------------------------------------------

static char *aboutWinText[] = {
  "X     X           d   fff",
  " X   X            d  f   f",
  "  X X             d  f",
  "   X    pppp   dddd ffff",
  "  X X   p   p d   d  f",
  " X   X  p   p d   d  f",
  "X     X pppp   dddd  f      Version " xpdfVersion,
  "        p",
  "        p",
  "        p",
  "",
  xpdfCopyright,
  "",
  "http://www.foolabs.com/xpdf/",
  "derekn@foolabs.com",
  "",
  "Licensed under the GNU General Public License (GPL).",
  "See the 'COPYING' file for details.",
  "",
  "Supports PDF version " supportedPDFVersionStr ".",
  "",
  "The PDF data structures, operators, and specification",
  "are copyright 1985-2001 Adobe Systems Inc.",
  "",
  "Mouse bindings:",
  "  button 1: select text / follow link",
  "  button 2: pan window",
  "  button 3: menu",
  "",
  "Key bindings:",
  "  o              = open file",
  "  r              = reload",
  "  f              = find text",
  "  n = <PgDn>     = next page",
  "  p = <PgUp>     = previous page",
  "  <space>        = scroll down",
  "  <backspace>    = <delete> = scroll up",
  "  v              = forward (history path)",
  "  b              = backward (history path)",
  "  0 / + / -      = zoom zero / in / out",
  "  z / w          = zoom page / page width",
  "  ctrl-L         = redraw",
  "  q              = quit",
  "  <home> / <end> = top / bottom of page",
  "  <arrows>       = scroll",
  "",
  "For more information, please read the xpdf(1) man page.",
  NULL
};

//------------------------------------------------------------------------
// command line options
//------------------------------------------------------------------------

static char initialZoomStr[32] = "";
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
static GBool doRemoteRaise = gFalse;
static GBool doRemoteQuit = gFalse;
static GBool printCommands = gFalse;
static GBool quiet = gFalse;
static char cfgFileName[256] = "";
static GBool printVersion = gFalse;
static GBool printHelp = gFalse;
static GBool viKeys = gFalse;

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
  {"-z",          argString,      initialZoomStr, sizeof(initialZoomStr),
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
   "paper size (letter, legal, A4, A3)"},
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

static XrmOptionDescRec opts[] = {
  {"-display",       ".display",         XrmoptionSepArg,  NULL},
  {"-foreground",    ".foreground",      XrmoptionSepArg,  NULL},
  {"-fg",            ".foreground",      XrmoptionSepArg,  NULL},
  {"-background",    ".background",      XrmoptionSepArg,  NULL},
  {"-bg",            ".background",      XrmoptionSepArg,  NULL},
  {"-geometry",      ".geometry",        XrmoptionSepArg,  NULL},
  {"-g",             ".geometry",        XrmoptionSepArg,  NULL},
  {"-font",          ".font",            XrmoptionSepArg,  NULL},
  {"-fn",            ".font",            XrmoptionSepArg,  NULL},
  {"-title",         ".title",           XrmoptionSepArg,  NULL},
  {"-cmap",          ".installCmap",     XrmoptionNoArg,   (XPointer)"on"},
  {"-rgb",           ".rgbCubeSize",     XrmoptionSepArg,  NULL},
  {"-rv",            ".reverseVideo",    XrmoptionNoArg,   (XPointer)"true"},
  {"-papercolor",    ".paperColor",      XrmoptionSepArg,  NULL},
  {NULL}
};

//------------------------------------------------------------------------
// global variables
//------------------------------------------------------------------------

// zoom factor is 1.2 (similar to DVI magsteps)
#define minZoom    -5
#define maxZoom     5
#define zoomPage  100
#define zoomWidth 101
#define defZoom     1
static int zoomDPI[maxZoom - minZoom + 1] = {
  29, 35, 42, 50, 60,
  72,
  86, 104, 124, 149, 179
};

static PDFDoc *doc;

static LTKOutputDev *out;

static int page;
static int zoom;
static int rotate;
static GBool quit;

static time_t modTime;		// last modification time of PDF file

static LinkAction *linkAction;	// mouse pointer is over this link
static int			// coordinates of current selection:
  selectXMin, selectYMin,	//   (xMin==xMax || yMin==yMax) means there
  selectXMax, selectYMax;	//   is no selection
static GBool lastDragLeft;	// last dragged selection edge was left/right
static GBool lastDragTop;	// last dragged selection edge was top/bottom
static int panMX, panMY;	// last mouse position for pan

struct History {
  GString *fileName;
  int page;
};
#define historySize 50
static History			// page history queue
  history[historySize];
static int historyCur;		// currently displayed page
static int historyBLen;		// number of valid entries backward from
				//   current entry
static int historyFLen;		// number of valid entries forward from
				//   current entry

static GString *psFileName;
static int psFirstPage, psLastPage;

static GString *fileReqDir;	// current directory for file requesters

static GString *urlCommand;	// command to execute for URI links

static GString *windowTitle;	// window title string

static LTKApp *app;
static Display *display;
static LTKWindow *win;
static LTKMenu *zoomMenu;
static LTKScrollingCanvas *canvas;
static LTKScrollbar *hScrollbar, *vScrollbar;
static LTKTextIn *pageNumText;
static LTKLabel *numPagesLabel;
static LTKLabel *linkLabel;
static LTKMenuButton *zoomMenuBtn;
static LTKWindow *aboutWin;
static LTKList *aboutList;
static LTKScrollbar *aboutHScrollbar, *aboutVScrollbar;
static LTKWindow *psDialog;
static LTKWindow *openDialog;
static LTKWindow *saveDialog;
static LTKWindow *findWin;
static LTKTextIn *findTextIn;
static Atom remoteAtom;
static GC selectGC;

//------------------------------------------------------------------------
// GUI includes
//------------------------------------------------------------------------

#include "xpdfIcon.xpm"
#include "leftArrow.xbm"
#include "dblLeftArrow.xbm"
#include "rightArrow.xbm"
#include "dblRightArrow.xbm"
#include "backArrow.xbm"
#include "forwardArrow.xbm"
#include "find.xbm"
#include "postscript.xbm"
#include "about.xbm"
#include "xpdf-ltk.h"

//------------------------------------------------------------------------
// main program
//------------------------------------------------------------------------

int main(int argc, char *argv[]) {
  Window xwin;
  XGCValues gcValues;
  char cmd[remoteCmdLength];
  LTKMenu *menu;
  GString *name;
  GString *title;
  GString *initialZoom;
  GBool reverseVideo;
  unsigned long paperColor;
  GBool installCmap;
  int rgbCubeSize;
  int pg;
  GString *destName;
  LinkDest *dest;
  int x, y;
  Guint width, height;
  double width1, height1;
  GBool ok;
  char s[20];
  int i;
  int ret;

  // initialize
  app = NULL;
  win = NULL;
  out = NULL;
  remoteAtom = None;
  doc = NULL;
  psFileName = NULL;
  fileReqDir = getCurrentDir();
  ret = 0;

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
  if (initialZoomStr[0]) {
    globalParams->setInitialZoom(initialZoomStr);
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
  if (quiet) {
    globalParams->setErrQuiet(quiet);
  }

  // create LTKApp (and parse X-related args)
  app = new LTKApp("xpdf", "Xpdf", opts, &argc, argv);
  app->setKillCbk(&killCbk);
  display = app->getDisplay();

  // check command line
  if (doRemoteRaise)
    ok = ok && remoteName[5] && !doRemoteQuit && argc >= 1 && argc <= 3;
  else if (doRemoteQuit)
    ok = ok && remoteName[5] && argc == 1;
  else
    ok = ok && argc >= 1 && argc <= 3;
  if (!ok || printVersion || printHelp) {
    fprintf(stderr, "xpdf version %s\n", xpdfVersion);
    fprintf(stderr, "%s\n", xpdfCopyright);
    if (!printVersion) {
      printUsage("xpdf", "[<PDF-file> [<page> | +<dest>]]", argDesc);
    }
    ret = 1;
    goto done2;
  }
  if (argc >= 2) {
    name = new GString(argv[1]);
  } else {
    name = NULL;
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

  // look for already-running remote server
  if (remoteName[5]) {
    remoteAtom = XInternAtom(display, remoteName, False);
    xwin = XGetSelectionOwner(display, remoteAtom);
    if (xwin != None) {
      if (name) {
	if (destName) {
	  sprintf(cmd, "%c +%.256s %.200s", doRemoteRaise ? 'D' : 'd',
		  destName->getCString(), name->getCString());
	  delete destName;
	} else {
	  sprintf(cmd, "%c %d %.200s", doRemoteRaise ? 'D' : 'd',
		  pg, name->getCString());
	}
	XChangeProperty(display, xwin, remoteAtom, remoteAtom, 8,
			PropModeReplace, (Guchar *)cmd, strlen(cmd) + 1);
	delete name;
      } else if (doRemoteRaise) {
	XChangeProperty(display, xwin, remoteAtom, remoteAtom, 8,
			PropModeReplace, (Guchar *)"r", 2);
      } else if (doRemoteQuit) {
	XChangeProperty(display, xwin, remoteAtom, remoteAtom, 8,
			PropModeReplace, (Guchar *)"q", 2);
      }
      goto done2;
    }
    if (doRemoteQuit) {
      if (destName) {
	delete destName;
      }
      goto done2;
    }
  }

  // no history yet
  historyCur = historySize - 1;
  historyBLen = historyFLen = 0;
  for (i = 0; i < historySize; ++i)
    history[i].fileName = NULL;

  // open PDF file
  if (name) {
    if (!loadFile(name)) {
      ret = 1;
      goto done1;
    }
    delete fileReqDir;
    fileReqDir = makePathAbsolute(grabPath(name->getCString()));
  }

  // check for legal page number
  if (doc && (pg < 1 || pg > doc->getNumPages())) {
    pg = 1;
  }

  // create window
  menu = makeMenu();
  if (fullScreen) {
    zoomMenu = NULL;
    win = makeFullScreenWindow(app);
    win->setDecorated(gFalse);
  } else {
    zoomMenu = makeZoomMenu();
    win = makeWindow(app);
  }
  win->setMenu(menu);
  canvas = (LTKScrollingCanvas *)win->findWidget("canvas");
  hScrollbar = (LTKScrollbar *)win->findWidget("hScrollbar");
  vScrollbar = (LTKScrollbar *)win->findWidget("vScrollbar");
  pageNumText = (LTKTextIn *)win->findWidget("pageNum");
  numPagesLabel = (LTKLabel *)win->findWidget("numPages");
  linkLabel = (LTKLabel *)win->findWidget("link");
  zoomMenuBtn = (LTKMenuButton *)win->findWidget("zoom");
  win->setKeyCbk(&keyPressCbk);
  win->setLayoutCbk(&layoutCbk);
  canvas->setButtonPressCbk(&buttonPressCbk);
  canvas->setButtonReleaseCbk(&buttonReleaseCbk);
  canvas->setMouseMoveCbk(&mouseMoveCbk);
  canvas->setMouseDragCbk(&mouseDragCbk);
  if (!fullScreen) {
    hScrollbar->setRepeatPeriod(0);
    vScrollbar->setRepeatPeriod(0);
  }

  // get parameters
  urlCommand = globalParams->getURLCommand();

  // get X resources
  windowTitle = app->getStringResource("title", NULL);
  installCmap = app->getBoolResource("installCmap", gFalse);
  if (installCmap) {
    win->setInstallCmap(gTrue);
  }
  rgbCubeSize = app->getIntResource("rgbCubeSize", defaultRGBCube);
  reverseVideo = app->getBoolResource("reverseVideo", gFalse);
  paperColor = app->getColorResource(
		  "paperColor",
		  reverseVideo ? (char *)"black" : (char *)"white",
		  reverseVideo ? BlackPixel(display, app->getScreenNum())
			       : WhitePixel(display, app->getScreenNum()),
		  NULL);
  if (fullScreen) {
    zoom = zoomPage;
  } else {
    initialZoom = globalParams->getInitialZoom();
    if (!initialZoom->cmp("page")) {
      zoom = zoomPage;
      i = maxZoom - minZoom + 2;
    } else if (!initialZoom->cmp("width")) {
      zoom = zoomWidth;
      i = maxZoom - minZoom + 3;
    } else {
      zoom = atoi(initialZoom->getCString());
      if (zoom < minZoom) {
	zoom = minZoom;
      } else if (zoom > maxZoom) {
	zoom = maxZoom;
      }
      i = zoom - minZoom;
    }
    zoomMenuBtn->setInitialMenuItem(zoomMenu->getItem(i));
  }
  viKeys = app->getBoolResource("viKeys", gFalse);

  // get geometry
  if (fullScreen) {
    x = y = 0;
    width = app->getDisplayWidth();
    height = app->getDisplayHeight();
  } else {
    x = y = -1;
    width = height = 0;
    app->getGeometryResource("geometry", &x, &y, &width, &height);
    if (width == 0 || height == 0) {
      if (!doc || doc->getNumPages() == 0) {
	width1 = 612;
	height1 = 792;
      } else if (doc->getPageRotate(pg) == 90 ||
		 doc->getPageRotate(pg) == 270) {
	width1 = doc->getPageHeight(pg);
	height1 = doc->getPageWidth(pg);
      } else {
	width1 = doc->getPageWidth(pg);
	height1 = doc->getPageHeight(pg);
      }
      if (zoom == zoomPage || zoom == zoomWidth) {
	width = (int)((width1 * zoomDPI[defZoom - minZoom]) / 72 + 0.5);
	height = (int)((height1 * zoomDPI[defZoom - minZoom]) / 72 + 0.5);
      } else {
	width = (int)((width1 * zoomDPI[zoom - minZoom]) / 72 + 0.5);
	height = (int)((height1 * zoomDPI[zoom - minZoom]) / 72 + 0.5);
      }
      width += 28;
      height += 56;
      if (width > (Guint)app->getDisplayWidth() - 100) {
	width = app->getDisplayWidth() - 100;
      }
      if (height > (Guint)app->getDisplayHeight() - 100) {
	height = app->getDisplayHeight() - 100;
      }
    }
  }

  // finish setting up window
  if (!fullScreen) {
    sprintf(s, "of %d", doc ? doc->getNumPages() : 0);
    numPagesLabel->setText(s);
  }
  if (windowTitle) {
    title = windowTitle->copy();
  } else if (name) {
    title = new GString("xpdf: ");
    title->append(name);
  } else {
    title = new GString("xpdf");
  }
  win->setTitle(title);
  win->layout(x, y, width, height);
  win->map();
  aboutWin = NULL;
  psDialog = NULL;
  openDialog = NULL;
  saveDialog = NULL;
  findWin = NULL;
  gcValues.foreground = BlackPixel(display, win->getScreenNum()) ^
                        WhitePixel(display, win->getScreenNum());
  gcValues.function = GXxor;
  selectGC = XCreateGC(display, win->getXWindow(),
		       GCForeground | GCFunction, &gcValues);

  // set up remote server
  if (remoteAtom != None) {
    win->setPropChangeCbk(&propChangeCbk);
    xwin = win->getXWindow();
    XSetSelectionOwner(display, remoteAtom, xwin, CurrentTime);
  }

  // create output device
  out = new LTKOutputDev(win, reverseVideo, paperColor,
			 installCmap, rgbCubeSize, !fullScreen);
  out->startDoc(doc ? doc->getXRef() : (XRef *)NULL);

  // display first page
  if (destName) {
    if (doc) {
      if ((dest = doc->findDest(destName))) {
	displayDest(dest, zoom, 0, gTrue);
	delete dest;
      } else {
	error(-1, "Unknown named destination '%s'", destName->getCString());
	displayPage(1, zoom, 0, gTrue);
      }
    }
    delete destName;
  } else {
    displayPage(pg, zoom, 0, gTrue);
  }

  // event loop
  quit = gFalse;
  do {
    app->doEvent(gTrue);
  } while (!quit);

 done1:
  // release remote control atom
  if (remoteAtom != None)
    XSetSelectionOwner(display, remoteAtom, None, CurrentTime);

 done2:
  // free stuff
  if (out) {
    delete out;
  }
  if (win) {
    delete win;
  }
  if (aboutWin) {
    delete aboutWin;
  }
  if (findWin) {
    delete findWin;
  }
  if (app) {
    delete app;
  }
  if (doc) {
    delete doc;
  }
  if (psFileName) {
    delete psFileName;
  }
  if (fileReqDir) {
    delete fileReqDir;
  }
  if (windowTitle) {
    delete windowTitle;
  }
  for (i = 0; i < historySize; ++i) {
    if (history[i].fileName) {
      delete history[i].fileName;
    }
  }
  delete globalParams;

  // check for memory leaks
  Object::memCheck(stderr);
  gMemReport(stderr);

  return ret;
}

//------------------------------------------------------------------------
// loadFile / displayPage / displayDest
//------------------------------------------------------------------------

static GBool loadFile(GString *fileName) {
  GString *title;
  PDFDoc *newDoc;
  GString *ownerPW, *userPW;
  char s[20];
  char *p;

  // busy cursor
  if (win)
    win->setBusyCursor(gTrue);

  // open PDF file
  if (ownerPassword[0]) {
    ownerPW = new GString(ownerPassword);
  } else {
    ownerPW = NULL;
  }
  if (userPassword[0]) {
    userPW = new GString(userPassword);
  } else {
    userPW = NULL;
  }
  newDoc = new PDFDoc(fileName, ownerPW, userPW, printCommands);
  if (userPW) {
    delete userPW;
  }
  if (ownerPW) {
    delete ownerPW;
  }
  if (!newDoc->isOk()) {
    delete newDoc;
    if (win)
      win->setBusyCursor(gFalse);
    return gFalse;
  }

  // replace old document
  if (doc)
    delete doc;
  doc = newDoc;
  if (out) {
    out->startDoc(doc->getXRef());
  }

  // nothing displayed yet
  page = -99;

  // save the modification time
  modTime = getModTime(fileName->getCString());

  // init PostScript output params
  if (psFileName)
    delete psFileName;
  if (globalParams->getPSFile()) {
    psFileName = globalParams->getPSFile()->copy();
  } else {
    p = fileName->getCString() + fileName->getLength() - 4;
    if (!strcmp(p, ".pdf") || !strcmp(p, ".PDF"))
      psFileName = new GString(fileName->getCString(),
			       fileName->getLength() - 4);
    else
      psFileName = fileName->copy();
    psFileName->append(".ps");
  }
  psFirstPage = 1;
  psLastPage = doc->getNumPages();

  // set up title, number-of-pages display; back to normal cursor
  if (win) {
    if (!windowTitle) {
      title = new GString("xpdf: ");
      title->append(fileName);
      win->setTitle(title);
    }
    if (!fullScreen) {
      sprintf(s, "of %d", doc->getNumPages());
      numPagesLabel->setText(s);
    }
    win->setBusyCursor(gFalse);
  }

  // done
  return gTrue;
}

static void displayPage(int pageA, int zoomA, int rotateA, GBool addToHist) {
  time_t modTime1;
  double hDPI, vDPI, dpi;
  int rot;
  char s[20];
  History *h;

  // check for document
  if (!doc || doc->getNumPages() == 0)
    return;

  // check for changes to the file
  modTime1 = getModTime(doc->getFileName()->getCString());
  if (modTime1 != modTime) {
    if (loadFile(doc->getFileName()->copy())) {
      if (pageA > doc->getNumPages()) {
	pageA = doc->getNumPages();
      }
    }
    modTime = modTime1;
  }

  // busy cursor
  if (win)
    win->setBusyCursor(gTrue);

  // new page/zoom/rotate values
  page = pageA;
  zoom = zoomA;
  rotate = rotateA;

  // initialize mouse-related stuff
  linkAction = NULL;
  win->setDefaultCursor();
  if (!fullScreen) {
    linkLabel->setText(NULL);
  }
  selectXMin = selectXMax = 0;
  selectYMin = selectYMax = 0;
  lastDragLeft = lastDragTop = gTrue;

  // draw the page
  rot = rotate + doc->getPageRotate(page);
  if (rot >= 360)
    rot -= 360;
  else if (rotate < 0)
    rot += 360;
  if (fullScreen) {
    if (rot == 90 || rot == 270) {
      hDPI = (win->getWidth() / doc->getPageHeight(page)) * 72;
      vDPI = (win->getHeight() / doc->getPageWidth(page)) * 72;
    } else {
      hDPI = (win->getWidth() / doc->getPageWidth(page)) * 72;
      vDPI = (win->getHeight() / doc->getPageHeight(page)) * 72;
    }
    dpi = (hDPI < vDPI) ? hDPI : vDPI;
  } else if (zoom == zoomPage) {
    if (rot == 90 || rot == 270) {
      hDPI = (canvas->getWidth() / doc->getPageHeight(page)) * 72;
      vDPI = (canvas->getHeight() / doc->getPageWidth(page)) * 72;
    } else {
      hDPI = (canvas->getWidth() / doc->getPageWidth(page)) * 72;
      vDPI = (canvas->getHeight() / doc->getPageHeight(page)) * 72;
    }
    dpi = (hDPI < vDPI) ? hDPI : vDPI;
  } else if (zoom == zoomWidth) {
    if (rot == 90 || rot == 270) {
      dpi = (canvas->getWidth() / doc->getPageHeight(page)) * 72;
    } else {
      dpi = (canvas->getWidth() / doc->getPageWidth(page)) * 72;
    }
  } else {
    dpi = zoomDPI[zoom - minZoom];
  }
  doc->displayPage(out, page, dpi, rotate, gTrue);
  updateScrollbars();

  // update page number display
  if (!fullScreen) {
    sprintf(s, "%d", page);
    pageNumText->setText(s);
  }

  // add to history
  if (addToHist) {
    if (++historyCur == historySize)
      historyCur = 0;
    h = &history[historyCur];
    if (h->fileName)
      delete h->fileName;
    h->fileName = doc->getFileName()->copy();
    h->page = page;
    if (historyBLen < historySize)
      ++historyBLen;
    historyFLen = 0;
  }

  // back to regular cursor
  win->setBusyCursor(gFalse);
}

static void displayDest(LinkDest *dest, int zoomA, int rotateA,
			GBool addToHist) {
  Ref pageRef;
  int pg;
  int dx, dy;

  if (dest->isPageRef()) {
    pageRef = dest->getPageRef();
    pg = doc->findPage(pageRef.num, pageRef.gen);
  } else {
    pg = dest->getPageNum();
  }
  if (pg <= 0 || pg > doc->getNumPages()) {
    pg = 1;
  }
  if (pg != page) {
    displayPage(pg, zoomA, rotateA, addToHist);
  }

  if (fullScreen) {
    return;
  }
  switch (dest->getKind()) {
  case destXYZ:
    out->cvtUserToDev(dest->getLeft(), dest->getTop(), &dx, &dy);
    if (dest->getChangeLeft() || dest->getChangeTop()) {
      if (dest->getChangeLeft()) {
	hScrollbar->setPos(dx, canvas->getWidth());
      }
      if (dest->getChangeTop()) {
	vScrollbar->setPos(dy, canvas->getHeight());
      }
      canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
    }
    //~ what is the zoom parameter?
    break;
  case destFit:
  case destFitB:
    //~ do fit
    hScrollbar->setPos(0, canvas->getWidth());
    vScrollbar->setPos(0, canvas->getHeight());
    canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
    break;
  case destFitH:
  case destFitBH:
    //~ do fit
    out->cvtUserToDev(0, dest->getTop(), &dx, &dy);
    hScrollbar->setPos(0, canvas->getWidth());
    vScrollbar->setPos(dy, canvas->getHeight());
    canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
    break;
  case destFitV:
  case destFitBV:
    //~ do fit
    out->cvtUserToDev(dest->getLeft(), 0, &dx, &dy);
    hScrollbar->setPos(dx, canvas->getWidth());
    vScrollbar->setPos(0, canvas->getHeight());
    canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
    break;
  case destFitR:
    //~ do fit
    out->cvtUserToDev(dest->getLeft(), dest->getTop(), &dx, &dy);
    hScrollbar->setPos(dx, canvas->getWidth());
    vScrollbar->setPos(dy, canvas->getHeight());
    canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
    break;
  }
}

//------------------------------------------------------------------------
// key press and menu callbacks
//------------------------------------------------------------------------

static void keyPressCbk(LTKWindow *win1, KeySym key, Guint modifiers,
			char *s, int n) {
  if (n > 0) {
    switch (s[0]) {
    case 'O':
    case 'o':
      mapOpenDialog();
      break;
    case 'R':
    case 'r':
      reloadCbk();
      break;
    case 'F':
    case 'f':
      mapFindWin();
      break;
    case 'N':
    case 'n':
      gotoNextPage(1, !(modifiers & Mod5Mask));
      break;
    case 'P':
    case 'p':
      gotoPrevPage(1, !(modifiers & Mod5Mask), gFalse);
      break;
    case ' ':
      if (fullScreen ||
	  vScrollbar->getPos() >=
	    canvas->getRealHeight() - canvas->getHeight()) {
	gotoNextPage(1, gTrue);
      } else {
	vScrollbar->setPos(vScrollbar->getPos() + canvas->getHeight(),
			   canvas->getHeight());
	canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
      }
      break;
    case '\b':			// bs
    case '\177':		// del
      if (fullScreen) {
	gotoPrevPage(1, gTrue, gFalse);
      } else if (vScrollbar->getPos() == 0) {
	gotoPrevPage(1, gFalse, gTrue);
      } else {
	vScrollbar->setPos(vScrollbar->getPos() - canvas->getHeight(),
			   canvas->getHeight());
	canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
      }
      break;
    case 'v':
      forwardCbk(NULL, 0, gTrue);
      break;
    case 'b':
      backCbk(NULL, 0, gTrue);
      break;
    case 'g':
      if (fullScreen) {
	break;
      }
      pageNumText->selectAll();
      pageNumText->activate(gTrue);
      break;
    case 'h':			// vi-style left
      if (fullScreen) {
	break;
      }
      if (viKeys) {
	hScrollbar->setPos(hScrollbar->getPos() - 16, canvas->getWidth());
	canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
      }
      break;
    case 'l':			// vi-style right
      if (fullScreen) {
	break;
      }
      if (viKeys) {
	hScrollbar->setPos(hScrollbar->getPos() + 16, canvas->getWidth());
	canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
      }
      break;
    case 'k':			// vi-style up
      if (fullScreen) {
	break;
      }
      if (viKeys) {
	vScrollbar->setPos(vScrollbar->getPos() - 16, canvas->getHeight());
	canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
      }
      break;
    case 'j':			// vi-style down
      if (fullScreen) {
	break;
      }
      if (viKeys) {
	vScrollbar->setPos(vScrollbar->getPos() + 16, canvas->getHeight());
	canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
      }
      break;
    case '0':
      if (fullScreen) {
	break;
      }
      if (zoom != defZoom) {
	zoomMenuBtn->setMenuItem(zoomMenu->getItem(defZoom - minZoom));
      }
      break;
    case '+':
      if (fullScreen) {
	break;
      }
      if (zoom >= minZoom && zoom < maxZoom) {
	zoomMenuBtn->setMenuItem(zoomMenu->getItem(zoom + 1 - minZoom));
      }
      break;
    case '-':
      if (fullScreen) {
	break;
      }
      if (zoom > minZoom && zoom <= maxZoom) {
	zoomMenuBtn->setMenuItem(zoomMenu->getItem(zoom - 1 - minZoom));
      }
      break;
    case 'z':
      if (fullScreen) {
	break;
      }
      zoomMenuBtn->setMenuItem(zoomMenu->getItem(maxZoom - minZoom + 2));
      break;
    case 'w':
      if (fullScreen) {
	break;
      }
      zoomMenuBtn->setMenuItem(zoomMenu->getItem(maxZoom - minZoom + 3));
      break;
    case '\014':		// ^L
      win->redraw();
      displayPage(page, zoom, rotate, gFalse);
      break;
    case 'Q':
    case 'q':
      quitCbk(NULL, 0, gTrue);
      break;
    }
  } else {
    switch (key) {
    case XK_Home:
    case XK_KP_Home:
      if (fullScreen) {
	break;
      }
      hScrollbar->setPos(0, canvas->getWidth());
      vScrollbar->setPos(0, canvas->getHeight());
      canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
      break;
    case XK_End:
    case XK_KP_End:
      if (fullScreen) {
	break;
      }
      hScrollbar->setPos(canvas->getRealWidth() - canvas->getWidth(),
			 canvas->getWidth());
      vScrollbar->setPos(canvas->getRealHeight() - canvas->getHeight(),
			 canvas->getHeight());
      canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
      break;
    case XK_Page_Up:
    case XK_KP_Page_Up:
      if (fullScreen) {
	gotoPrevPage(1, gTrue, gFalse);
      } else if (vScrollbar->getPos() == 0) {
	gotoPrevPage(1, gFalse, gTrue);
      } else {
	vScrollbar->setPos(vScrollbar->getPos() - canvas->getHeight(),
			   canvas->getHeight());
	canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
      }
      break;
    case XK_Page_Down:
    case XK_KP_Page_Down:
      if (fullScreen ||
	  vScrollbar->getPos() >=
	    canvas->getRealHeight() - canvas->getHeight()) {
	gotoNextPage(1, gTrue);
      } else {
	vScrollbar->setPos(vScrollbar->getPos() + canvas->getHeight(),
			   canvas->getHeight());
	canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
      }
      break;
    case XK_Left:
    case XK_KP_Left:
      if (fullScreen) {
	break;
      }
      hScrollbar->setPos(hScrollbar->getPos() - 16, canvas->getWidth());
      canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
      break;
    case XK_Right:
    case XK_KP_Right:
      if (fullScreen) {
	break;
      }
      hScrollbar->setPos(hScrollbar->getPos() + 16, canvas->getWidth());
      canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
      break;
    case XK_Up:
    case XK_KP_Up:
      if (fullScreen) {
	break;
      }
      vScrollbar->setPos(vScrollbar->getPos() - 16, canvas->getHeight());
      canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
      break;
    case XK_Down:
    case XK_KP_Down:
      if (fullScreen) {
	break;
      }
      vScrollbar->setPos(vScrollbar->getPos() + 16, canvas->getHeight());
      canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
      break;
    }
  }
}

static void menuCbk(LTKMenuItem *item) {
  int r;

  switch (item->getItemNum()) {
  case menuOpen:
    mapOpenDialog();
    break;
  case menuReload:
    reloadCbk();
    break;
  case menuSavePDF:
    if (doc)
      mapSaveDialog();
    break;
  case menuRotateCCW:
    if (doc) {
      r = (rotate == 0) ? 270 : rotate - 90;
      displayPage(page, zoom, r, gFalse);
    }
    break;
  case menuRotateCW:
    if (doc) {
      r = (rotate == 270) ? 0 : rotate + 90;
      displayPage(page, zoom, r, gFalse);
    }
    break;
  case menuQuit:
    quit = gTrue;
    break;
  }
}

//------------------------------------------------------------------------
// mouse callbacks
//------------------------------------------------------------------------

static void buttonPressCbk(LTKWidget *canvas1, int n,
			   int mx, int my, int button, GBool dblClick) {
  if (!doc || doc->getNumPages() == 0) {
    return;
  }
  switch (button) {
  case 1:
    setSelection(mx, my, mx, my);
    break;
  case 2:
    if (!fullScreen) {
      panMX = mx - hScrollbar->getPos();
      panMY = my - vScrollbar->getPos();
    }
    break;
  case 4: // mouse wheel up
    if (fullScreen) {
      gotoPrevPage(1, gTrue, gFalse);
    } else if (vScrollbar->getPos() == 0) {
      gotoPrevPage(1, gFalse, gTrue);
    } else {
      vScrollbar->setPos(vScrollbar->getPos() - 16, canvas->getHeight());
      canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
    }
    break;
  case 5: // mouse wheel down
    if (fullScreen ||
	vScrollbar->getPos() >=
	canvas->getRealHeight() - canvas->getHeight()) {
      gotoNextPage(1, gTrue);
    } else {
      vScrollbar->setPos(vScrollbar->getPos() + 16, canvas->getHeight());
      canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
    }
    break;
  case 6: // second mouse wheel right
    if (fullScreen) {
      return;
    }
    hScrollbar->setPos(hScrollbar->getPos() + 16, canvas->getWidth());
    canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
    break;
  case 7: // second mouse wheel left
    if (fullScreen) {
      return;
    }
    hScrollbar->setPos(hScrollbar->getPos() - 16, canvas->getWidth());
    canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
    break;
  }
}

static void buttonReleaseCbk(LTKWidget *canvas1, int n,
			     int mx, int my, int button, GBool click) {
  GString *s;

  if (!doc || doc->getNumPages() == 0)
    return;

  if (button == 1) {
    // selection
    if (selectXMin < selectXMax && selectYMin < selectYMax) {
#ifndef NO_TEXT_SELECT
#ifdef ENFORCE_PERMISSIONS
      if (doc->okToCopy()) {
#endif
	s = out->getText(selectXMin, selectYMin, selectXMax, selectYMax);
	win->setSelection(NULL, s);
#ifdef ENFORCE_PERMISSIONS
      } else {
	error(-1, "Copying of text from this document is not allowed.");
      }
#endif
#endif

    // link
    } else {
      setSelection(mx, my, mx, my);
      doLink(mx, my);
    }
  }
}

static void doLink(int mx, int my) {
  LinkActionKind kind;
  LinkAction *action = NULL;
  LinkDest *dest;
  GString *namedDest;
  char *s;
  GString *fileName;
  GString *actionName;
  double x, y;
  LTKButtonDialog *dialog;
  int i;

  // look for a link
  out->cvtDevToUser(mx, my, &x, &y);
  if ((action = doc->findLink(x, y))) {
    switch (kind = action->getKind()) {

    // GoTo / GoToR action
    case actionGoTo:
    case actionGoToR:
      if (kind == actionGoTo) {
	dest = NULL;
	namedDest = NULL;
	if ((dest = ((LinkGoTo *)action)->getDest()))
	  dest = dest->copy();
	else if ((namedDest = ((LinkGoTo *)action)->getNamedDest()))
	  namedDest = namedDest->copy();
      } else {
	dest = NULL;
	namedDest = NULL;
	if ((dest = ((LinkGoToR *)action)->getDest()))
	  dest = dest->copy();
	else if ((namedDest = ((LinkGoToR *)action)->getNamedDest()))
	  namedDest = namedDest->copy();
	s = ((LinkGoToR *)action)->getFileName()->getCString();
	//~ translate path name for VMS (deal with '/')
	if (isAbsolutePath(s))
	  fileName = new GString(s);
	else
	  fileName = appendToPath(
			 grabPath(doc->getFileName()->getCString()), s);
	if (!loadFile(fileName)) {
	  if (dest)
	    delete dest;
	  if (namedDest)
	    delete namedDest;
	  return;
	}
      }
      if (namedDest) {
	dest = doc->findDest(namedDest);
	delete namedDest;
      }
      if (dest) {
	displayDest(dest, zoom, rotate, gTrue);
	delete dest;
      } else {
	if (kind == actionGoToR) {
	  displayPage(1, zoom, 0, gTrue);
	}
      }
      break;

    // Launch action
    case actionLaunch:
      fileName = ((LinkLaunch *)action)->getFileName();
      s = fileName->getCString();
      if (!strcmp(s + fileName->getLength() - 4, ".pdf") ||
	  !strcmp(s + fileName->getLength() - 4, ".PDF")) {
	//~ translate path name for VMS (deal with '/')
	if (isAbsolutePath(s))
	  fileName = fileName->copy();
	else
	  fileName = appendToPath(
		         grabPath(doc->getFileName()->getCString()), s);
	if (!loadFile(fileName))
	  return;
	displayPage(1, zoom, rotate, gTrue);
      } else {
	fileName = fileName->copy();
	if (((LinkLaunch *)action)->getParams()) {
	  fileName->append(' ');
	  fileName->append(((LinkLaunch *)action)->getParams());
	}
#ifdef VMS
	fileName->insert(0, "spawn/nowait ");
#elif defined(__EMX__)
	fileName->insert(0, "start /min /n ");
#else
	fileName->append(" &");
#endif
	dialog = new LTKButtonDialog(win, "xpdf: Launch",
				     "Execute the command:",
				     fileName->getCString(),
				     NULL, "Ok", "Cancel");
	if (dialog->go())
	  system(fileName->getCString());
	delete dialog;
	delete fileName;
      }
      break;

    // URI action
    case actionURI:
      if (urlCommand) {
	for (s = urlCommand->getCString(); *s; ++s) {
	  if (s[0] == '%' && s[1] == 's') {
	    break;
	  }
	}
	if (s) {
	  fileName = ((LinkURI *)action)->getURI()->copy();
	  // filter out any quote marks (' or ") to avoid a potential
	  // security hole
	  i = 0;
	  while (i < fileName->getLength()) {
	    if (fileName->getChar(i) == '"') {
	      fileName->del(i);
	      fileName->insert(i, "%22");
	      i += 3;
	    } else if (fileName->getChar(i) == '\'') {
	      fileName->del(i);
	      fileName->insert(i, "%27");
	      i += 3;
	    } else {
	      ++i;
	    }
	  }
	  fileName->insert(0, urlCommand->getCString(),
			   s - urlCommand->getCString());
	  fileName->append(s+2);
	} else {
	  fileName = urlCommand->copy();
	}
#ifdef VMS
	fileName->insert(0, "spawn/nowait ");
#elif defined(__EMX__)
	fileName->insert(0, "start /min /n ");
#else
	fileName->append(" &");
#endif
	system(fileName->getCString());
	delete fileName;
      } else {
	printf("URI: %s\n", ((LinkURI *)action)->getURI()->getCString());
      }
      break;

    // Named action
    case actionNamed:
      actionName = ((LinkNamed *)action)->getName();
      if (!actionName->cmp("NextPage")) {
	gotoNextPage(1, gTrue);
      } else if (!actionName->cmp("PrevPage")) {
	gotoPrevPage(1, gTrue, gFalse);
      } else if (!actionName->cmp("FirstPage")) {
	if (page != 1) {
	  displayPage(1, zoom, rotate, gTrue);
	}
      } else if (!actionName->cmp("LastPage")) {
	if (page != doc->getNumPages()) {
	  displayPage(doc->getNumPages(), zoom, rotate, gTrue);
	}
      } else if (!actionName->cmp("GoBack")) {
	backCbk(NULL, 0, gTrue);
      } else if (!actionName->cmp("GoForward")) {
	forwardCbk(NULL, 0, gTrue);
      } else if (!actionName->cmp("Quit")) {
	quitCbk(NULL, 0, gTrue);
      } else {
	error(-1, "Unknown named action: '%s'", actionName->getCString());
      }
      break;

    // unknown action type
    case actionUnknown:
      error(-1, "Unknown link action type: '%s'",
	    ((LinkUnknown *)action)->getAction()->getCString());
      break;
    }
  }
}

static void mouseMoveCbk(LTKWidget *widget, int widgetNum, int mx, int my) {
  double x, y;
  LinkAction *action;
  char *s;

  if (!doc || doc->getNumPages() == 0)
    return;
  out->cvtDevToUser(mx, my, &x, &y);
  if ((action = doc->findLink(x, y))) {
    if (action != linkAction) {
      if (!linkAction) {
	win->setCursor(XC_hand2);
      }
      linkAction = action;
      if (!fullScreen) {
	s = NULL;
	switch (linkAction->getKind()) {
	case actionGoTo:
	  s = "[internal link]";
	  break;
	case actionGoToR:
	  s = ((LinkGoToR *)linkAction)->getFileName()->getCString();
	  break;
	case actionLaunch:
	  s = ((LinkLaunch *)linkAction)->getFileName()->getCString();
	  break;
	case actionURI:
	  s = ((LinkURI *)action)->getURI()->getCString();
	  break;
	case actionNamed:
	  s = ((LinkNamed *)linkAction)->getName()->getCString();
	  break;
	case actionUnknown:
	  s = "[unknown link]";
	  break;
	}
	linkLabel->setText(s);
      }
    }
  } else {
    if (linkAction) {
      linkAction = NULL;
      win->setDefaultCursor();
      if (!fullScreen) {
	linkLabel->setText(NULL);
      }
    }
  }
}

static void mouseDragCbk(LTKWidget *widget, int widgetNum,
			 int mx, int my, int button) {
  int x, y;
  int xMin, yMin, xMax, yMax;

  // button 1: select
  if (button == 1) {

    // clip mouse coords
    x = mx;
    if (x < 0)
      x = 0;
    else if (x >= canvas->getRealWidth())
      x = canvas->getRealWidth() - 1;
    y = my;
    if (y < 0)
      y = 0;
    else if (y >= canvas->getRealHeight())
      y = canvas->getRealHeight() - 1;

    // move appropriate edges of selection
    if (lastDragLeft) {
      if (x < selectXMax) {
	xMin = x;
	xMax = selectXMax;
      } else {
	xMin = selectXMax;
	xMax = x;
	lastDragLeft = gFalse;
      }      
    } else {
      if (x > selectXMin) {
	xMin = selectXMin;
	xMax = x;
      } else {
	xMin = x;
	xMax = selectXMin;
	lastDragLeft = gTrue;
      }
    }
    if (lastDragTop) {
      if (y < selectYMax) {
	yMin = y;
	yMax = selectYMax;
      } else {
	yMin = selectYMax;
	yMax = y;
	lastDragTop = gFalse;
      }
    } else {
      if (y > selectYMin) {
	yMin = selectYMin;
	yMax = y;
      } else {
	yMin = y;
	yMax = selectYMin;
	lastDragTop = gTrue;
      }
    }

    // redraw the selection
    setSelection(xMin, yMin, xMax, yMax);

  // button 2: pan
  } else if (!fullScreen && button == 2) {
    mx -= hScrollbar->getPos();
    my -= vScrollbar->getPos();
    hScrollbar->setPos(hScrollbar->getPos() - (mx - panMX),
		       canvas->getWidth());
    vScrollbar->setPos(vScrollbar->getPos() - (my - panMY),
		       canvas->getHeight());
    canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
    panMX = mx;
    panMY = my;
  }
}

//------------------------------------------------------------------------
// button callbacks
//------------------------------------------------------------------------

static void nextPageCbk(LTKWidget *button, int n, GBool on) {
  gotoNextPage(1, gTrue);
}

static void nextTenPageCbk(LTKWidget *button, int n, GBool on) {
  gotoNextPage(10, gTrue);
}

static void gotoNextPage(int inc, GBool top) {
  int pg;

  if (!doc || doc->getNumPages() == 0) {
    return;
  }
  if (page < doc->getNumPages()) {
    if (top && !fullScreen) {
      vScrollbar->setPos(0, canvas->getHeight());
      canvas->setScrollPos(hScrollbar->getPos(), vScrollbar->getPos());
    }
    if ((pg = page + inc) > doc->getNumPages()) {
      pg = doc->getNumPages();
    }
    displayPage(pg, zoom, rotate, gTrue);
  } else {
    XBell(display, 0);
  }
}

static void prevPageCbk(LTKWidget *button, int n, GBool on) {
  gotoPrevPage(1, gTrue, gFalse);
}

static void prevTenPageCbk(LTKWidget *button, int n, GBool on) {
  gotoPrevPage(10, gTrue, gFalse);
}

static void gotoPrevPage(int dec, GBool top, GBool bottom) {
  int pg;

  if (!doc || doc->getNumPages() == 0) {
    return;
  }
  if (page > 1) {
    if (top && !fullScreen) {
      vScrollbar->setPos(0, canvas->getHeight());
      canvas->setScrollPos(hScrollbar->getPos(), vScrollbar->getPos());
    } else if (bottom && !fullScreen) {
      vScrollbar->setPos(canvas->getRealHeight() - canvas->getHeight(),
			 canvas->getHeight());
      canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
    }
    if ((pg = page - dec) < 1) {
      pg = 1;
    }
    displayPage(pg, zoom, rotate, gTrue);
  } else {
    XBell(display, 0);
  }
}

static void backCbk(LTKWidget *button, int n, GBool on) {
  if (historyBLen <= 1) {
    XBell(display, 0);
    return;
  }
  if (--historyCur < 0)
    historyCur = historySize - 1;
  --historyBLen;
  ++historyFLen;
  if (history[historyCur].fileName->cmp(doc->getFileName()) != 0) {
    if (!loadFile(history[historyCur].fileName->copy())) {
      XBell(display, 0);
      return;
    }
  }
  displayPage(history[historyCur].page, zoom, rotate, gFalse);
}

static void forwardCbk(LTKWidget *button, int n, GBool on) {
  if (historyFLen == 0) {
    XBell(display, 0);
    return;
  }
  if (++historyCur == historySize)
    historyCur = 0;
  --historyFLen;
  ++historyBLen;
  if (history[historyCur].fileName->cmp(doc->getFileName()) != 0) {
    if (!loadFile(history[historyCur].fileName->copy())) {
      XBell(display, 0);
      return;
    }
  }
  displayPage(history[historyCur].page, zoom, rotate, gFalse);
}

static void pageNumCbk(LTKWidget *textIn, int n, GString *text) {
  int page1;
  char s[20];

  if (!doc || doc->getNumPages() == 0)
    return;
  page1 = atoi(text->getCString());
  if (page1 >= 1 && page1 <= doc->getNumPages()) {
    if (page1 != page)
      displayPage(page1, zoom, rotate, gTrue);
  } else {
    XBell(display, 0);
    sprintf(s, "%d", page);
    pageNumText->setText(s);
  }
}

static void zoomMenuCbk(LTKMenuItem *item) {
  int z;

  if (!doc || doc->getNumPages() == 0) {
    return;
  }
  z = item->getItemNum();
  if (z != zoom) {
    displayPage(page, z, rotate, gFalse);
  }
}

static void postScriptCbk(LTKWidget *button, int n, GBool on) {
  if (!doc)
    return;
  mapPSDialog();
}

static void aboutCbk(LTKWidget *button, int n, GBool on) {
  mapAboutWin();
}

static void quitCbk(LTKWidget *button, int n, GBool on) {
  quit = gTrue;
}

//------------------------------------------------------------------------
// scrollbar callbacks
//------------------------------------------------------------------------

static void scrollVertCbk(LTKWidget *scrollbar, int n, int val) {
  canvas->scroll(hScrollbar->getPos(), val);
  XSync(display, False);
}

static void scrollHorizCbk(LTKWidget *scrollbar, int n, int val) {
  canvas->scroll(val, vScrollbar->getPos());
  XSync(display, False);
}

//------------------------------------------------------------------------
// misc callbacks
//------------------------------------------------------------------------

static void layoutCbk(LTKWindow *win1) {
  if (page >= 0 && (zoom == zoomPage || zoom == zoomWidth)) {
    displayPage(page, zoom, rotate, gFalse);
  } else {
    updateScrollbars();
  }
}

static void updateScrollbars() {
  if (fullScreen) {
    return;
  }
  hScrollbar->setLimits(0, canvas->getRealWidth() - 1);
  hScrollbar->setPos(hScrollbar->getPos(), canvas->getWidth());
  hScrollbar->setScrollDelta(16);
  vScrollbar->setLimits(0, canvas->getRealHeight() - 1);
  vScrollbar->setPos(vScrollbar->getPos(), canvas->getHeight());
  vScrollbar->setScrollDelta(16);
  canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
}

static void propChangeCbk(LTKWindow *win1, Atom atom) {
  Window xwin;
  char *cmd;
  Atom type;
  int format;
  Gulong size, remain;
  char *p, *q;
  GString *newFileName;
  int newPage;
  GString *destName;
  LinkDest *dest;

  // get command
  xwin = win1->getXWindow();
  if (XGetWindowProperty(display, xwin, remoteAtom,
			 0, remoteCmdLength/4, True, remoteAtom,
			 &type, &format, &size, &remain,
			 (Guchar **)&cmd) != Success) {
    return;
  }
  if (size == 0) {
    return;
  }

  // raise window
  if (cmd[0] == 'D' || cmd[0] == 'r'){
    win->raise();
    XFlush(display);
  }

  // display file / page
  if (cmd[0] == 'd' || cmd[0] == 'D') {
    p = cmd + 2;
    q = strchr(p, ' ');
    if (!q) {
      return;
    }
    *q++ = '\0';
    newPage = 1;
    destName = NULL;
    if (*p == '+') {
      destName = new GString(p + 1);
    } else {
      newPage = atoi(p);
    }
    if (!q) {
      return;
    }
    newFileName = new GString(q);
    XFree((XPointer)cmd);
    if (!doc || newFileName->cmp(doc->getFileName())) {
      if (!loadFile(newFileName))
	return;
    } else {
      delete newFileName;
    }
    if (destName) {
      if ((dest = doc->findDest(destName))) {
	displayDest(dest, zoom, rotate, gTrue);
	delete dest;
      }
      delete destName;
    } else if (newPage != page &&
	       newPage >= 1 && newPage <= doc->getNumPages()) {
      displayPage(newPage, zoom, rotate, gTrue);
    }

  // quit
  } else if (cmd[0] == 'q') {
    quit = gTrue;
  }
}

//------------------------------------------------------------------------
// selection
//------------------------------------------------------------------------

static void setSelection(int newXMin, int newYMin, int newXMax, int newYMax) {
  int x, y, w, h;
  GBool needRedraw, needScroll;
  GBool moveLeft, moveRight, moveTop, moveBottom;

  // erase old selection on canvas pixmap
  needRedraw = gFalse;
  if (selectXMin < selectXMax && selectYMin < selectYMax) {
    XFillRectangle(canvas->getDisplay(), canvas->getPixmap(),
		   selectGC, selectXMin, selectYMin,
		   selectXMax - selectXMin, selectYMax - selectYMin);
    needRedraw = gTrue;
  }

  // draw new selection on canvas pixmap
  if (newXMin < newXMax && newYMin < newYMax) {
    XFillRectangle(canvas->getDisplay(), canvas->getPixmap(),
		   selectGC, newXMin, newYMin,
		   newXMax - newXMin, newYMax - newYMin);
    needRedraw = gTrue;
  }

  // check which edges moved
  moveLeft = newXMin != selectXMin;
  moveTop = newYMin != selectYMin;
  moveRight = newXMax != selectXMax;
  moveBottom = newYMax != selectYMax;

  // redraw currently visible part of canvas
  if (needRedraw) {
    if (moveLeft) {
      canvas->redrawRect((newXMin < selectXMin) ? newXMin : selectXMin,
			 (newYMin < selectYMin) ? newYMin : selectYMin,
			 (newXMin > selectXMin) ? newXMin : selectXMin,
			 (newYMax > selectYMax) ? newYMax : selectYMax);
    }
    if (moveRight) {
      canvas->redrawRect((newXMax < selectXMax) ? newXMax : selectXMax,
			 (newYMin < selectYMin) ? newYMin : selectYMin,
			 (newXMax > selectXMax) ? newXMax : selectXMax,
			 (newYMax > selectYMax) ? newYMax : selectYMax);
    }
    if (moveTop) {
      canvas->redrawRect((newXMin < selectXMin) ? newXMin : selectXMin,
			 (newYMin < selectYMin) ? newYMin : selectYMin,
			 (newXMax > selectXMax) ? newXMax : selectXMax,
			 (newYMin > selectYMin) ? newYMin : selectYMin);
    }
    if (moveBottom) {
      canvas->redrawRect((newXMin < selectXMin) ? newXMin : selectXMin,
			 (newYMax < selectYMax) ? newYMax : selectYMax,
			 (newXMax > selectXMax) ? newXMax : selectXMax,
			 (newYMax > selectYMax) ? newYMax : selectYMax);
    }
  }

  // switch to new selection coords
  selectXMin = newXMin;
  selectXMax = newXMax;
  selectYMin = newYMin;
  selectYMax = newYMax;

  // scroll canvas if necessary
  if (fullScreen) {
    return;
  }
  needScroll = gFalse;
  w = canvas->getWidth();
  h = canvas->getHeight();
  x = hScrollbar->getPos();
  y = vScrollbar->getPos();
  if (moveLeft && selectXMin < x) {
    x = selectXMin;
    needScroll = gTrue;
  } else if (moveRight && selectXMax >= x + w) {
    x = selectXMax - w;
    needScroll = gTrue;
  } else if (moveLeft && selectXMin >= x + w) {
    x = selectXMin - w;
    needScroll = gTrue;
  } else if (moveRight && selectXMax < x) {
    x = selectXMax;
    needScroll = gTrue;
  }
  if (moveTop && selectYMin < y) {
    y = selectYMin;
    needScroll = gTrue;
  } else if (moveBottom && selectYMax >= y + h) {
    y = selectYMax - h;
    needScroll = gTrue;
  } else if (moveTop && selectYMin >= y + h) {
    y = selectYMin - h;
    needScroll = gTrue;
  } else if (moveBottom && selectYMax < y) {
    y = selectYMax;
    needScroll = gTrue;
  }
  if (needScroll) {
    hScrollbar->setPos(x, w);
    vScrollbar->setPos(y, h);
    canvas->scroll(x, y);
  }
}

//------------------------------------------------------------------------
// "Open" dialog
//------------------------------------------------------------------------

static void mapOpenDialog() {
  openDialog = makeOpenDialog(app);
  ((LTKFileReq *)openDialog->findWidget("fileReq"))->setDir(fileReqDir);
  openDialog->layoutDialog(win, -1, -1);
  openDialog->map();
}

static void openButtonCbk(LTKWidget *button, int n, GBool on) {
  LTKFileReq *fileReq;
  GString *sel;

  sel = NULL;
  if (n == 1) {
    fileReq = (LTKFileReq *)openDialog->findWidget("fileReq");
    if (!(sel = fileReq->getSelection())) {
      return;
    }
    openSelectCbk(fileReq, 0, sel);
  }
  if (openDialog) {
    if (sel) {
      delete fileReqDir;
      fileReqDir = ((LTKFileReq *)openDialog->findWidget("fileReq"))->getDir();
    }
    delete openDialog;
    openDialog = NULL;
  }
}

static void openSelectCbk(LTKWidget *widget, int n, GString *name) {
  GString *name1;

  name1 = name->copy();
  if (openDialog) {
    delete fileReqDir;
    fileReqDir = ((LTKFileReq *)openDialog->findWidget("fileReq"))->getDir();
    delete openDialog;
    openDialog = NULL;
  }
  if (loadFile(name1)) {
    if (!fullScreen) {
      vScrollbar->setPos(0, canvas->getHeight());
      canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
    }
    displayPage(1, zoom, rotate, gTrue);
  }
}

//------------------------------------------------------------------------
// "Reload"
//------------------------------------------------------------------------

static void reloadCbk() {
  int pg;

  if (!doc) {
    return;
  }
  pg = page;
  if (loadFile(doc->getFileName()->copy())) {
    if (pg > doc->getNumPages()) {
      pg = doc->getNumPages();
    }
    displayPage(pg, zoom, rotate, gFalse);
  }
}

//------------------------------------------------------------------------
// "Save PDF" dialog
//------------------------------------------------------------------------

static void mapSaveDialog() {
  saveDialog = makeSaveDialog(app);
  ((LTKFileReq *)saveDialog->findWidget("fileReq"))->setDir(fileReqDir);
  saveDialog->layoutDialog(win, -1, -1);
  saveDialog->map();
}

static void saveButtonCbk(LTKWidget *button, int n, GBool on) {
  LTKFileReq *fileReq;
  GString *sel;

  if (!doc)
    return;
  sel = NULL;
  if (n == 1) {
    fileReq = (LTKFileReq *)saveDialog->findWidget("fileReq");
    if (!(sel = fileReq->getSelection())) {
      return;
    }
    saveSelectCbk(fileReq, 0, sel);
  }
  if (saveDialog) {
    if (sel) {
      delete fileReqDir;
      fileReqDir = ((LTKFileReq *)saveDialog->findWidget("fileReq"))->getDir();
    }
    delete saveDialog;
    saveDialog = NULL;
  }
}

static void saveSelectCbk(LTKWidget *widget, int n, GString *name) {
  GString *name1;

  name1 = name->copy();
  if (saveDialog) {
    delete fileReqDir;
    fileReqDir = ((LTKFileReq *)saveDialog->findWidget("fileReq"))->getDir();
    delete saveDialog;
    saveDialog = NULL;
  }
  win->setBusyCursor(gTrue);
  doc->saveAs(name1);
  delete name1;
  win->setBusyCursor(gFalse);
}

//------------------------------------------------------------------------
// "PostScript" dialog
//------------------------------------------------------------------------

static void mapPSDialog() {
  LTKTextIn *widget;
  char s[20];

  psDialog = makePostScriptDialog(app);
  sprintf(s, "%d", psFirstPage);
  widget = (LTKTextIn *)psDialog->findWidget("firstPage");
  widget->setText(s);
  sprintf(s, "%d", psLastPage);
  widget = (LTKTextIn *)psDialog->findWidget("lastPage");
  widget->setText(s);
  widget = (LTKTextIn *)psDialog->findWidget("fileName");
  widget->setText(psFileName->getCString());
  psDialog->layoutDialog(win, -1, -1);
  psDialog->map();
}

static void psButtonCbk(LTKWidget *button, int n, GBool on) {
  PSOutputDev *psOut;
  LTKTextIn *widget;

  if (!doc)
    return;

  // "Ok" button
  if (n == 1) {
    // extract params and close the dialog
    widget = (LTKTextIn *)psDialog->findWidget("firstPage");
    psFirstPage = atoi(widget->getText()->getCString());
    if (psFirstPage < 1)
      psFirstPage = 1;
    widget = (LTKTextIn *)psDialog->findWidget("lastPage");
    psLastPage = atoi(widget->getText()->getCString());
    if (psLastPage < psFirstPage)
      psLastPage = psFirstPage;
    else if (psLastPage > doc->getNumPages())
      psLastPage = doc->getNumPages();
    widget = (LTKTextIn *)psDialog->findWidget("fileName");
    if (psFileName)
      delete psFileName;
    psFileName = widget->getText()->copy();
    if (!(psFileName->getChar(0) == '|' ||
	  psFileName->cmp("-") == 0))
      makePathAbsolute(psFileName);

    // do the PostScript output
    psDialog->setBusyCursor(gTrue);
    win->setBusyCursor(gTrue);
#ifdef ENFORCE_PERMISSIONS
    if (doc->okToPrint()) {
#endif
      psOut = new PSOutputDev(psFileName->getCString(), doc->getXRef(),
			      doc->getCatalog(), psFirstPage, psLastPage,
			      psModePS);
      if (psOut->isOk()) {
	doc->displayPages(psOut, psFirstPage, psLastPage, 72, 0, gFalse);
      }
      delete psOut;
#ifdef ENFORCE_PERMISSIONS
    } else {
      error(-1, "Printing this document is not allowed.");
    }
#endif

    delete psDialog;
    win->setBusyCursor(gFalse);

  // "Cancel" button
  } else {
    delete psDialog;
  }
}

//------------------------------------------------------------------------
// "About" window
//------------------------------------------------------------------------

static void mapAboutWin() {
  int i;

  if (aboutWin) {
    aboutWin->raise();
  } else {
    aboutWin = makeAboutWindow(app);
    aboutWin->setLayoutCbk(&aboutLayoutCbk);
    aboutList = (LTKList *)aboutWin->findWidget("list");
    aboutHScrollbar = (LTKScrollbar *)aboutWin->findWidget("hScrollbar");
    aboutVScrollbar = (LTKScrollbar *)aboutWin->findWidget("vScrollbar");
    for (i = 0; aboutWinText[i]; ++i) {
      aboutList->addLine(aboutWinText[i]);
    }
    aboutWin->layout(-1, -1, -1, -1);
    aboutWin->map();
  }
}

static void closeAboutCbk(LTKWidget *button, int n, GBool on) {
  delete aboutWin;
  aboutWin = NULL;
}

static void aboutLayoutCbk(LTKWindow *winA) {
  aboutHScrollbar->setLimits(0, aboutList->getMaxWidth() - 1);
  aboutHScrollbar->setPos(aboutHScrollbar->getPos(), aboutList->getWidth());
  aboutVScrollbar->setLimits(0, aboutList->getNumLines() - 1);
  aboutVScrollbar->setPos(aboutVScrollbar->getPos(),
			  aboutList->getDisplayedLines());
}

static void aboutScrollVertCbk(LTKWidget *scrollbar, int n, int val) {
  aboutList->scrollTo(val, aboutHScrollbar->getPos());
  XSync(display, False);
}

static void aboutScrollHorizCbk(LTKWidget *scrollbar, int n, int val) {
  aboutList->scrollTo(aboutVScrollbar->getPos(), val);
  XSync(display, False);
}

//------------------------------------------------------------------------
// "Find" window
//------------------------------------------------------------------------

static void findCbk(LTKWidget *button, int n, GBool on) {
  if (!doc || doc->getNumPages() == 0)
    return;
  mapFindWin();
}

static void mapFindWin() {
  if (findWin) {
    findWin->raise();
  } else {
    findWin = makeFindWindow(app);
    findWin->layout(-1, -1, -1, -1);
    findWin->map();
    findTextIn = (LTKTextIn *)findWin->findWidget("text");
  }
  findTextIn->activate(gTrue);
}

static void findButtonCbk(LTKWidget *button, int n, GBool on) {
  if (!doc || doc->getNumPages() == 0)
    return;
  if (n == 1) {
    doFind(findTextIn->getText()->getCString());
  } else {
    delete findWin;
    findWin = NULL;
  }
}

static void doFind(char *s) {
  Unicode *u;
  TextOutputDev *textOut;
  int xMin, yMin, xMax, yMax;
  double xMin1, yMin1, xMax1, yMax1;
  int pg;
  GBool top;
  GString *s1;
  int len, i;

  // check for zero-length string
  if (!s[0]) {
    XBell(display, 0);
    return;
  }

  // set cursors to watch
  win->setBusyCursor(gTrue);
  findWin->setBusyCursor(gTrue);

  // convert to Unicode
#if 1 //~ should do something more intelligent here
  len = strlen(s);
  u = (Unicode *)gmalloc(len * sizeof(Unicode));
  for (i = 0; i < len; ++i) {
    u[i] = (Unicode)(s[i] & 0xff);
  }
#endif

  // search current page starting at current selection or top of page
  xMin = yMin = xMax = yMax = 0;
  if (selectXMin < selectXMax && selectYMin < selectYMax) {
    xMin = selectXMax;
    yMin = (selectYMin + selectYMax) / 2;
    top = gFalse;
  } else {
    top = gTrue;
  }
  if (out->findText(u, len, top, gTrue, &xMin, &yMin, &xMax, &yMax)) {
    goto found;
  }

  // search following pages
  textOut = new TextOutputDev(NULL, gFalse, gFalse);
  if (!textOut->isOk()) {
    delete textOut;
    goto done;
  }
  for (pg = page+1; pg <= doc->getNumPages(); ++pg) {
    doc->displayPage(textOut, pg, 72, 0, gFalse);
    if (textOut->findText(u, len, gTrue, gTrue,
			  &xMin1, &yMin1, &xMax1, &yMax1)) {
      goto foundPage;
    }
  }

  // search previous pages
  for (pg = 1; pg < page; ++pg) {
    doc->displayPage(textOut, pg, 72, 0, gFalse);
    if (textOut->findText(u, len, gTrue, gTrue,
			  &xMin1, &yMin1, &xMax1, &yMax1)) {
      goto foundPage;
    }
  }
  delete textOut;

  // search current page ending at current selection
  if (selectXMin < selectXMax && selectYMin < selectYMax) {
    xMax = selectXMin;
    yMax = (selectYMin + selectYMax) / 2;
    if (out->findText(u, len, gTrue, gFalse, &xMin, &yMin, &xMax, &yMax)) {
      goto found;
    }
  }

  // not found
  XBell(display, 0);
  goto done;

  // found on a different page
 foundPage:
  delete textOut;
  displayPage(pg, zoom, rotate, gTrue);
  if (!out->findText(u, len, gTrue, gTrue, &xMin, &yMin, &xMax, &yMax)) {
    // this can happen if coalescing is bad
    goto done;
  }

  // found: change the selection
 found:
  setSelection(xMin, yMin, xMax, yMax);
#ifndef NO_TEXT_SELECT
#ifdef ENFORCE_PERMISSIONS
  if (doc->okToCopy()) {
#endif
    s1 = out->getText(selectXMin, selectYMin, selectXMax, selectYMax);
    win->setSelection(NULL, s1);
#ifdef ENFORCE_PERMISSIONS
  }
#endif
#endif

 done:
  gfree(u);

  // reset cursors to normal
  win->setBusyCursor(gFalse);
  findWin->setBusyCursor(gFalse);
}

//------------------------------------------------------------------------
// app kill callback
//------------------------------------------------------------------------

static void killCbk(LTKWindow *win1) {
  if (win1 == win) {
    quit = gTrue;
  } else if (win1 == aboutWin) {
    delete aboutWin;
    aboutWin = NULL;
  } else if (win1 == psDialog) {
    delete psDialog;
    psDialog = NULL;
  } else if (win1 == openDialog) {
    delete openDialog;
    openDialog = NULL;
  } else if (win1 == saveDialog) {
    delete saveDialog;
    saveDialog = NULL;
  } else if (win1 == findWin) {
    delete findWin;
    findWin = NULL;
  }
}
