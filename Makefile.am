# just set the variable
appsdir = $(kde_appsdir)/Graphics
# set the include path for X, qt and KDE
INCLUDES= $(all_includes) $(KDVI_KPATHSEA_INC) -I$(top_srcdir)/kviewshell
# claim, which subdirectories you want to install
SUBDIRS = $(KDVI_KPATHSEA_DIR) . pix

# you can add here more. This one gets installed
lib_LTLIBRARIES= libkdvi.la
noinst_PROGRAMS = squeeze

# just to make sure, automake makes them
METASOURCES = AUTO

libkdvi_la_SOURCES = kdvi_multipage.cpp \
	dviwin.cpp print.cpp \
	printData.cpp printSetup.cpp printSetupData.cpp\
	optiondialog.cpp \
	psheader.c dviwin_draw.cpp dvi_init.cpp\
	font.cpp pk.cpp psgs.cpp\
	special.cpp util.cpp vf.cpp glyph.cpp

libkdvi_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkdvi_la_LIBADD = $(LIB_KFILE) -lkparts $(KDVI_KPATHSEA_LIB) \
			$(top_builddir)/kviewshell/libkmultipage.la

# Which sources should be compiled for squeeze.
squeeze_SOURCES	= squeeze.c


## this option you can leave out. Just, if you use "make dist", you need it
noinst_HEADERS = c-openmx.h dvi.h dviwin.h oconfig.h\
	print.h printData.h printSetup.h printSetupData.h\
	optiondialog.h xdvi.h 

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kdvi.pot

apps_DATA = kdvi.desktop


bin_SCRIPTS = kdvi

partdir = $(kde_datadir)/kdvi
part_DATA = kdvi_part.rc

psheader.c: psheader.txt squeeze
	./squeeze $(srcdir)/psheader.txt $@

# serial num for Makefile.am to retrigger remake: 1
