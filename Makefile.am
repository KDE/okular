
kpathsea = kpathsea/libkpathsea.la

# just set the variable
appsdir = $(kde_appsdir)/Programs/Graphics
# set the include path for X, qt and KDE
INCLUDES= $(all_includes) -Ikpathsea
# claim, which subdirectories you want to install
SUBDIRS = pix

# you can add here more. This one gets installed 
bin_PROGRAMS = 	kdvi
lib_LTLIBRARIES=libkdvi.la
noinst_PROGRAMS = squeeze

# just to make sure, automake makes them 
METASOURCES = USE_AUTOMOC
BUILT_SOURCES = psheader.c

# Which sources should be compiled for kdvi.
libkdvi_la_SOURCES = kdvi.cpp dviwin.cpp marklist.cpp prefs.cpp print.cpp\
	printData.cpp printSetup.cpp printSetupData.cpp pushbutton.cpp\
	scrbox.cpp kdvi_miniwidget.cpp kdvi_view.cpp kbrowser.moc.cc\
	psheader.c dvi_draw.c dvi_init.c font-open.c gf.c new.c pk.c psgs.c\
	special.c util.c vf.c konq_progressproxy.cc

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
	prefs.h print.h printData.h printSetup.h printSetupData.h pushbutton.h\
	scrbox.h xdvi.h kdvi_miniwidget.h kdvi_view.h konq_progressproxy.h

# if you "make distclean", this files get removed. If you want to remove
# them while "make clean", use CLEANFILES
CLEANFILES = $(BUILT_SOURCES)

messages:
	$(XGETTEXT) -C -ki18n -x $(includedir)/kde.pot *.cpp && mv messages.po ../po/kdvi.pot

apps_DATA = kdvi.desktop

$(srcdir)/psheader.c: psheader.txt squeeze $(kpathsea)
	./squeeze $(srcdir)/psheader.txt $@

# this is somehow a dirty hack to force the automatic dependency checking
# to create the stuff in the kpathsea directory first. vf.c is the last
# source file in the list.
vf.c: $(kpathsea)

$(kpathsea):
	test -d kpathsea || mkdir kpathsea
	oldpath=`pwd`; \
	cd $(srcdir) && newpath=`pwd`; \
	cd $$oldpath/kpathsea && CC="$(CC)" CFLAGS="$(CFLAGS)" MAKE="$(MAKE)" $$newpath/kpathsea/configure 
	cd kpathsea && $(MAKE)

kbrowser.moc.cc: $(kde_includes)/kbrowser.h
	$(MOC) $(kde_includes)/kbrowser.h -o $(top_builddir)/kdvi/kbrowser.moc.cc

