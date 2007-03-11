INCLUDES = -I$(srcdir)/.. -I$(srcdir)/../fofi -I$(srcdir)/../splash -I$(srcdir)/../goo $(LIBFREETYPE_CFLAGS) $(XFT_CFLAGS) $(all_includes)

libxpdf_la_LDFLAGS = $(all_libraries)
libxpdf_la_LIBADD = $(LIB_X11) $(LIBFREETYPE_LIBS) $(LIBPAPER_LIBS) $(XFT_LIBS) $(LIBJPEG) ../goo/libgoo.la ../fofi/libfofi.la ../splash/libsplash.la 
libxpdf_la_SOURCES = Annot.cc Array.cc BuiltinFont.cc BuiltinFontTables.cc \
     Catalog.cc CharCodeToUnicode.cc CMap.cc Decrypt.cc Dict.cc \
     FontEncodingTables.cc Function.cc Gfx.cc \
     GfxFont.cc GfxState.cc GlobalParams.cc JArithmeticDecoder.cc \
     JBIG2Stream.cc Lexer.cc Link.cc NameToCharCode.cc Object.cc Outline.cc \
     OutputDev.cc PDFDoc.cc PDFDocEncoding.cc PreScanOutputDev.cc PSTokenizer.cc \
     Page.cc Parser.cc PSOutputDev.cc SecurityHandler.cc SplashOutputDev.cc Stream.cc JPXStream.cc \
     TextOutputDev.cc UnicodeMap.cc UnicodeTypeTable.cc XRef.cc

noinst_LTLIBRARIES = libxpdf.la

# This fixes crash in Bug 109015 which i assume is a compiler bug
# as adding some correctly placed printf in SplashOutputDev::convertPath() makes this 
# option unneeded
KDE_CXXFLAGS=$(NOREGMOVE)
