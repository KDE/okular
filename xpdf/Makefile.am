INCLUDES = -I.. -I$(top_srcdir)/kpdf/goo $(all_includes) $(LIBFREETYPE_CFLAGS)

libxpdf_la_LDFLAGS = $(all_libraries) -no-undefined
libxpdf_la_LIBADD = $(LIB_X11) $(LIBFREETYPE_LIBS) $(LIBPAPER_LIBS) ../goo/libgoo.la
libxpdf_la_SOURCES = Annot.cc Array.cc BuiltinFont.cc BuiltinFontTables.cc \
     CMap.cc Catalog.cc CharCodeToUnicode.cc Decrypt.cc Dict.cc Error.cc \
     FTFont.cc FontEncodingTables.cc FontFile.cc Function.cc Gfx.cc \
     GfxFont.cc GfxState.cc GlobalParams.cc ImageOutputDev.cc \
     JBIG2Stream.cc Lexer.cc Link.cc NameToCharCode.cc Object.cc Outline.cc \
     OutputDev.cc PDFDoc.cc PDFDocEncoding.cc PSOutputDev.cc PSTokenizer.cc \
     Page.cc Parser.cc SFont.cc Stream.cc \
     TextOutputDev.cc UnicodeMap.cc XOutputDev.cc XRef.cc
libxpdf_la_COMPILE_FIRST = ../aconf.h

noinst_LTLIBRARIES = libxpdf.la
noinst_HEADERS     = Annot.h Array.h BuiltinFont.h BuiltinFontTables.h \
    CMap.h Catalog.h CharCodeToUnicode.h CharTypes.h CompactFontTables.h \
    Decrypt.h Dict.h DisplayFontTable.h Error.h ErrorCodes.h FTFont.h \
    FontEncodingTables.h FontFile.h Function.h Gfx.h GfxFont.h \
    GfxState.h GlobalParams.h ImageOutputDev.h JBIG2Stream.h LTKOutputDev.h \
    Lexer.h Link.h NameToCharCode.h NameToUnicodeTable.h Object.h Outline.h \
    OutputDev.h PBMOutputDev.h PDFDoc.h PDFDocEncoding.cc PSOutputDev.h \
    PSTokenizer.h Page.h Parser.h SFont.h Stream-CCITT.h Stream.h \
    TextOutputDev.h UTF8.h UnicodeMap.h UnicodeMapTables.h \
    XOutputDev.h XRef.h config.h xpdf-ltk.h

