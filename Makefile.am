# set the include path for X, qt and KDE
INCLUDES= -I$(top_srcdir)/kviewshell $(all_includes) $(LIBFREETYPE_CFLAGS)
# claim, which subdirectories you want to install
SUBDIRS = . pix

# you can add here more. This one gets installed
kde_module_LTLIBRARIES= kdvipart.la
noinst_PROGRAMS = squeeze

# just to make sure, automake makes them
METASOURCES = AUTO

kdvipart_la_SOURCES = kdvi_multipage.cpp kdvi_multipage_texthandling.cpp \
	dviwin.cpp bigEndianByteReader.cpp infodialog.cpp \
	psheader.c dviwin_draw.cpp dviwin_prescan.cpp dviwin_export.cpp \
	dviFile.cpp fontpool.cpp fontprogress.cpp psgs.cpp \
	fontMap.cpp fontEncoding.cpp fontEncodingPool.cpp \
	special.cpp util.cpp vf.cpp glyph.cpp selection.cpp history.cpp \
	optionDialogFontsWidget.cpp optionDialogFontsWidget_base.ui \
	optionDialogSpecialWidget.cpp optionDialogSpecialWidget_base.ui \
	optionDialogGUIWidget_base.ui \
	TeXFont.cpp TeXFont_PK.cpp TeXFont_PFB.cpp TeXFont_TFM.cpp \
	TeXFontDefinition.cpp \
	documentPage.cpp documentWidget.cpp documentPageCache.cpp \
	prefs.kcfgc

kde_kcfg_DATA = kdvi.kcfg

kdvipart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
kdvipart_la_LIBADD = $(LIBFREETYPE_LIBS) -lkdeprint -lkparts \
			$(top_builddir)/kviewshell/libkmultipage.la

# Which sources should be compiled for squeeze.
squeeze_SOURCES	= squeeze.c

KDE_OPTIONS = nofinal

## this option you can leave out. Just, if you use "make dist", you need it
noinst_HEADERS = dvi.h dviwin.h xdvi.h 

messages: rc.cpp
	$(PREPARETIPS) > tips.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kdvi.pot
	rm -f tips.cpp

xdg_apps_DATA = kdvi.desktop

tip_DATA = tips
tipdir = $(kde_datadir)/kdvi

bin_SCRIPTS = kdvi

partdir = $(kde_datadir)/kdvi
part_DATA = kdvi_part.rc

psheader.c: psheader.txt squeeze
	./squeeze $(srcdir)/psheader.txt $@

CLEANFILES = psheader.c
