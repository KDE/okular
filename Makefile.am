# set the include path for X, qt and KDE
INCLUDES= -I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/kviewshell/shell \
	-I$(top_builddir)/shell \
	$(all_includes) $(LIBFREETYPE_CFLAGS) $(CAIRO_CFLAGS)
# claim, which subdirectories you want to install
SUBDIRS = . pix

# you can add here more. This one gets installed
kde_module_LTLIBRARIES= kdvipart.la
noinst_PROGRAMS = squeeze

# just to make sure, automake makes them
METASOURCES = AUTO

kdvipart_la_SOURCES = \
	bigEndianByteReader.cpp \
	dviexport.cpp \
	dviFile.cpp \
	dviRenderer.cpp \
	dviRenderer_draw.cpp \
	dviRenderer_prescan.cpp \
	dvisourceeditor.cpp \
	dvisourcesplitter.cpp \
	dviWidget.cpp \
	fontEncoding.cpp \
	fontEncodingPool.cpp \
	fontMap.cpp \
	fontpool.cpp \
	fontprogress.cpp \
	glyph.cpp \
	infodialog.cpp \
	kdvi_multipage.cpp \
	kdvi_multipage_texthandling.cpp \
	optionDialogFontsWidget.cpp \
	optionDialogFontsWidget_base.ui \
	optionDialogSpecialWidget.cpp \
	optionDialogSpecialWidget_base.ui \
	prefs.kcfgc \
	psgs.cpp \
	psheader.c \
	renderedDviPagePixmap.cpp \
	special.cpp \
	TeXFont.cpp \
	TeXFont_PFB.cpp \
	TeXFont_PK.cpp \
	TeXFont_TFM.cpp \
	TeXFontDefinition.cpp \
	util.cpp \
	vf.cpp

kde_kcfg_DATA = kdvi.kcfg

kdvipart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
kdvipart_la_LIBADD = $(LIBFREETYPE_LIBS) $(CAIRO_LIBS) -lkparts \
			$(top_builddir)/kviewshell/shell/libkmultipage.la

# Which sources should be compiled for squeeze.
squeeze_SOURCES	= squeeze.c

KDE_OPTIONS = nofinal

## this option you can leave out. Just, if you use "make dist", you need it
noinst_HEADERS = dvi.h dviRenderer.h xdvi.h

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kdvi.pot

plugindir = $(kde_datadir)
plugin_DATA = kdvi_part.rc

kde_services_DATA = kdvimultipage.desktop

partdesktopdir = $(kde_servicesdir)
partdesktop_DATA = kvs_dvi_part.desktop

psheader.c: psheader.txt squeeze
	./squeeze $(srcdir)/psheader.txt $@

CLEANFILES = psheader.c
