
# just set the variable
appsdir = $(kde_appsdir)/Graphics
# set the include path for X, qt and KDE
INCLUDES= $(all_includes) -Ikpathsea
# claim, which subdirectories you want to install
SUBDIRS = kpathsea . pix

# you can add here more. This one gets installed
bin_PROGRAMS = 	kdvi
lib_LTLIBRARIES=libkdvi.la
noinst_PROGRAMS = squeeze

# just to make sure, automake makes them
METASOURCES = AUTO

# Which sources should be compiled for kdvi.
libkdvi_la_SOURCES = kdvi.cpp dviwin.cpp marklist.cpp print.cpp\
	printData.cpp printSetup.cpp printSetupData.cpp pushbutton.cpp\
	optiondialog.cpp gotodialog.cpp scrbox.cpp kdvi_miniwidget.cpp\
	kdvi_view.cpp psheader.c dviwin_draw.cpp dvi_init.cpp\
	font-open.cpp gf.cpp new.cpp pk.cpp psgs.cpp\
	special.cpp util.cpp vf.cpp glyph.cpp

kdvi_SOURCES = main.cpp

# Which sources should be compiled for squeeze.
squeeze_SOURCES	= squeeze.c

# the library search path
libkdvi_la_LDFLAGS = $(all_libraries) -avoid-version -module
libkdvi_la_LIBADD = $(LIB_KFILE) -lkparts kpathsea/libkpathsea.la

kdvi_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kdvi_LDADD = libkdvi.la

## this option you can leave out. Just, if you use "make dist", you need it
noinst_HEADERS = c-openmx.h dvi.h dviwin.h kdvi.h marklist.h oconfig.h\
	print.h printData.h printSetup.h printSetupData.h\
	pushbutton.h optiondialog.h gotodialog.h scrbox.h xdvi.h\
	 kdvi_miniwidget.h kdvi_view.h

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kdvi.pot

apps_DATA = kdvi.desktop

partdir = $(kde_datadir)/kdvi
part_DATA = kdvi_part.rc

psheader.c: psheader.txt squeeze
	./squeeze $(srcdir)/psheader.txt $@

