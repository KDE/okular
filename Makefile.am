# just set the variable
appsdir = $(kde_appsdir)/Graphics
# set the include path for X, qt and KDE
INCLUDES= -I$(top_srcdir)/kviewshell $(all_includes)
# claim, which subdirectories you want to install
SUBDIRS = . pix

# you can add here more. This one gets installed
kde_module_LTLIBRARIES= kdvipart.la
noinst_PROGRAMS = squeeze

# just to make sure, automake makes them
METASOURCES = AUTO

kdvipart_la_SOURCES = kdvi_multipage.cpp \
	dviwin.cpp bigEndianByteReader.cpp infodialog.cpp \
	psheader.c dviwin_draw.cpp dviwin_export.cpp dviwin_textsearch.cpp \
	dvi_init.cpp font.cpp fontpool.cpp fontprogress.cpp pk.cpp psgs.cpp \
	special.cpp util.cpp vf.cpp glyph.cpp selection.cpp history.cpp \
	optiondialog.cpp optionDialogFontsWidget.cpp optionDialogFontsWidget_base.ui \
	optionDialogSpecialWidget.cpp optionDialogSpecialWidget_base.ui

kdvipart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
kdvipart_la_LIBADD = -lkdeprint -lkparts \
			$(top_builddir)/kviewshell/libkmultipage.la

# Which sources should be compiled for squeeze.
squeeze_SOURCES	= squeeze.c


## this option you can leave out. Just, if you use "make dist", you need it
noinst_HEADERS = c-openmx.h dvi.h dviwin.h oconfig.h\
	optiondialog.h xdvi.h 

messages: rc.cpp
	$(PREPARETIPS) > tips.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kdvi.pot
	rm -f tips.cpp

apps_DATA = kdvi.desktop

tip_DATA = tips
tipdir = $(kde_datadir)/kdvi

bin_SCRIPTS = kdvi

partdir = $(kde_datadir)/kdvi
part_DATA = kdvi_part.rc

psheader.c: psheader.txt squeeze
	./squeeze $(srcdir)/psheader.txt $@
