INCLUDES = -I$(top_srcdir)/kpdf -I$(srcdir)/../fofi -I$(srcdir)/../goo $(LIBFREETYPE_CFLAGS)

libsplash_la_SOURCES = Splash.cc SplashBitmap.cc SplashClip.cc SplashFTFont.cc SplashFTFontEngine.cc \
	SplashFTFontFile.cc SplashFont.cc SplashFontEngine.cc SplashFontFile.cc SplashFontFileID.cc \
	SplashPath.cc SplashPattern.cc SplashScreen.cc SplashState.cc SplashT1Font.cc \
	SplashT1FontEngine.cc SplashT1FontFile.cc SplashXPath.cc SplashXPathScanner.cc
libsplash_la_COMPILE_FIRST = ../aconf.h

noinst_LTLIBRARIES = libsplash.la
