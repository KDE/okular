
kpathsea = kpathsea/kpathsea.a

# just set the variable
appsdir = $(kde_appsdir)/Graphics
# set the include path for X, qt and KDE
INCLUDES= $(all_includes) -Ikpathsea
# claim, which subdirectories you want to install
SUBDIRS = pix

# you can add here more. This one gets installed 
bin_PROGRAMS = 	kdvi
noinst_PROGRAMS = squeeze

# just to make sure, automake makes them 
kdvi_METASOURCES = USE_AUTOMOC
BUILT_SOURCES = psheader.c

# Which sources should be compiled for kdvi.
kdvi_SOURCES	= kdvi.cpp dviwin.cpp marklist.cpp prefs.cpp print.cpp\
	printData.cpp printSetup.cpp printSetupData.cpp pushbutton.cpp\
	scrbox.cpp \
	psheader.c dvi_draw.c dvi_init.c font-open.c gf.c new.c pk.c psgs.c\
	special.c util.c vf.c

# Which sources should be compiled for squeeze.
squeeze_SOURCES	= squeeze.c

# the library search path
kdvi_LDFLAGS = $(all_libraries) $(KDE_RPATH)

# the libraries to link against. Be aware of the order. First the libraries,
# that depend on the following ones.
kdvi_LDADD   = $(kpathsea) $(LIB_KFILE)

## this option you can leave out. Just, if you use "make dist", you need it
noinst_HEADERS = c-openmx.h dvi.h dviwin.h kdvi.h marklist.h oconfig.h\
	prefs.h print.h printData.h printSetup.h printSetupData.h pushbutton.h\
	scrbox.h xdvi.h

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

