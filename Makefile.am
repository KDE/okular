# set the include path for X, qt and KDE
INCLUDES= -I$(top_srcdir)/kviewshell \
	-I$(top_builddir)/kviewshell \
	$(all_includes) $(LIBFREETYPE_CFLAGS)
# claim, which subdirectories you want to install
SUBDIRS = . pix

bin_PROGRAMS = kdvi

# you can add here more. This one gets installed
kde_module_LTLIBRARIES= kdvipart.la
noinst_PROGRAMS = squeeze

# just to make sure, automake makes them
METASOURCES = AUTO

kdvipart_la_SOURCES = renderedDviPagePixmap.cpp dviPageCache.cpp \
	kdvi_multipage.cpp kdvi_multipage_texthandling.cpp \
	dviRenderer.cpp bigEndianByteReader.cpp infodialog.cpp \
	psheader.c dviRenderer_draw.cpp dviRenderer_prescan.cpp dviRenderer_export.cpp \
	dviFile.cpp fontpool.cpp fontprogress.cpp psgs.cpp \
	fontMap.cpp fontEncoding.cpp fontEncodingPool.cpp \
	special.cpp util.cpp vf.cpp glyph.cpp \
	optionDialogFontsWidget.cpp optionDialogFontsWidget_base.ui \
	optionDialogSpecialWidget.cpp optionDialogSpecialWidget_base.ui \
	TeXFont.cpp TeXFont_PK.cpp TeXFont_PFB.cpp TeXFont_TFM.cpp \
	TeXFontDefinition.cpp dviWidget.cpp dvisourcesplitter.cpp \
	prefs.kcfgc 

kde_kcfg_DATA = kdvi.kcfg

kdvipart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
kdvipart_la_LIBADD = $(LIBFREETYPE_LIBS) -lkparts \
			$(top_builddir)/kviewshell/libkmultipage.la

# Which sources should be compiled for squeeze.
squeeze_SOURCES	= squeeze.c

KDE_OPTIONS = nofinal

kdvi_SOURCES = main.cpp 
kdvi_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kdvi_LDADD = ../kviewshell/libifaces.la ../kviewshell/libkviewshell.la -lkparts

## this option you can leave out. Just, if you use "make dist", you need it
noinst_HEADERS = dvi.h dviRenderer.h xdvi.h

messages: rc.cpp
	$(PREPARETIPS) > tips.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kdvi.pot
	rm -f tips.cpp

xdg_apps_DATA = kdvi.desktop

tip_DATA = tips
tipdir = $(kde_datadir)/kdvi

partdir = $(kde_datadir)/kdvi
part_DATA = ../kviewshell/kviewshell.rc kdvi_part.rc

kde_services_DATA = kdvimultipage.desktop

psheader.c: psheader.txt squeeze
	./squeeze $(srcdir)/psheader.txt $@

CLEANFILES = psheader.c
