
kpathsea = $(srcdir)/kpathsea/STATIC/libkpathsea.a

# this 10 paths are KDE specific. Use them:
# kde_htmldir       Where your docs should go to. (contains lang subdirs)
# kde_appsdir       Where your application file (.kdelnk) should go to. 
# kde_icondir       Where your icon should go to.
# kde_minidir       Where your mini icon should go to.
# kde_datadir       Where you install application data. (Use a subdir)
# kde_locale        Where translation files should go to.(contains lang subdirs)
# kde_cgidir        Where cgi-bin executables should go to.
# kde_confdir       Where config files should go to.
# kde_mimedir       Where mimetypes should go to.
# kde_toolbardir    Where general toolbar icons should go to.
# kde_wallpaperdir  Where general wallpapers should go to.

# just set the variable
APPSDIR = $(kde_appsdir)/Graphics
# set the include path for X, qt and KDE
INCLUDES= $(all_includes) -Ikpathsea
# claim, which subdirectories you want to install
SUBDIRS = doc pix

# you can add here more. This one gets installed 
bin_PROGRAMS = 	kdvi
noinst_PROGRAMS = squeeze

# just to make sure, automake makes them 
kdvi_METASOURCES = USE_AUTOMOC
BUILT_SOURCES = psheader.c

# Which sources should be compiled for kdvi.
kdvi_SOURCES	= dviwin.cpp kdvi.cpp marklist.cpp prefs.cpp print.cpp\
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

# just install datas here. Use install-exec-data for scripts and etc.
# the binary itself is already installed from automake
# use mkinstalldirs, not "install -d"
# don't install a list of file. Just one file per install.
# if you have more of them, create a subdirectory with an extra Makefile 
install-data-local: 
	$(mkinstalldirs) $(APPSDIR)
	$(INSTALL_DATA) $(srcdir)/kdvi.kdelnk $(APPSDIR)
	$(mkinstalldirs) $(kde_icondir)
	$(INSTALL_DATA) $(srcdir)/pix/kdvi.xpm $(kde_icondir)
	$(mkinstalldirs) $(kde_icondir)/large
	$(INSTALL_DATA) $(srcdir)/pix/large-kdvi.xpm $(kde_icondir)/large/kdvi.xpm
	$(mkinstalldirs) $(kde_minidir)
	$(INSTALL_DATA) $(srcdir)/pix/mini-kdvi.xpm $(kde_minidir)/kdvi.xpm

# remove ALL you have installed in install-data-local or install-exec-local
uninstall-local:
	-rm -f $(APPSDIR)/Kdvi.kdelnk
	-rm -f $(kde_icondir)/kdvi.xpm
	-rm -f $(kde_icondir)/large/kdvi.xpm
	-rm -f $(kde_minidir)/kdvi.xpm

clean-local:
	(cd kpathsea && test -r Makefile && $(MAKE) distclean)

distclean-local:
	(cd kpathsea && test -r config.status && $(MAKE) distclean)

$(srcdir)/psheader.c: psheader.txt squeeze $(kpathsea)
	./squeeze $(srcdir)/psheader.txt $@

# this is somehow a dirty hack to force the automatic dependency checking
# to create the stuff in the kpathsea directory first. vf.c is the last
# source file in the list.
vf.c: $(kpathsea)
dviwin.cpp: $(kpathsea)

$(kpathsea):
	test -d kpathsea || mkdir kpathsea
	oldpath=`pwd`; \
	cd $(srcdir) && newpath=`pwd`; \
	cd $$oldpath/kpathsea && CC="$(CC)" CFLAGS="$(CFLAGS)" MAKE="$(MAKE)" $$newpath/kpathsea/configure 
	cd kpathsea && $(MAKE)

