
kpathsea = kpathsea/kpathsea.a

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
INCLUDES= @all_includes@ -Ikpathsea
# claim, which subdirectories you want to install
SUBDIRS = doc pix

# you can add here more. This one gets installed 
bin_PROGRAMS = 	kdvi
noinst_PROGRAMS = squeeze

# just to make sure, automake makes them 
BUILTSOURCES =	mdviwin.cpp mkdvi.cpp mmarklist.cpp mprefs.cpp mprint.cpp\
	mprintData.cpp mprintSetup.cpp mprintSetupData.cpp mpushbutton.cpp\
	mscrbox.cpp psheader.c

# Which sources should be compiled for kdvi.
kdvi_SOURCES	= dviwin.cpp kdvi.cpp marklist.cpp prefs.cpp print.cpp\
	printData.cpp printSetup.cpp printSetupData.cpp pushbutton.cpp\
	scrbox.cpp mdviwin.cpp mkdvi.cpp mmarklist.cpp mprefs.cpp mprint.cpp mprintData.cpp\
	mprintSetup.cpp mprintSetupData.cpp mpushbutton.cpp mscrbox.cpp\
	psheader.c dvi_draw.c dvi_init.c font-open.c gf.c new.c pk.c psgs.c\
	special.c util.c vf.c

# Which sources should be compiled for squeeze.
squeeze_SOURCES	= squeeze.c

# the library search path
kdvi_LDFLAGS = @all_libraries@ $(KDE_RPATH)

# the libraries to link against. Be aware of the order. First the libraries,
# that depend on the following ones.
kdvi_LDADD   = $(kpathsea)  -lkfm -lkdeui -lkdecore -lqt -lXext -lX11 @LIBSOCKET@


## this option you can leave out. Just, if you use "make dist", you need it
noinst_HEADERS = c-openmx.h dvi.h dviwin.h kdvi.h marklist.h oconfig.h\
	prefs.h print.h printData.h printSetup.h printSetupData.h pushbutton.h\
	scrbox.h xdvi.h

# if you "make distclean", this files get removed. If you want to remove
# them while "make clean", use CLEANFILES
DISTCLEANFILES = $(BUILTSOURCES)
CLEANFILES = $(BUILTSOURCES) squeeze kdvi 

# make messages.po. Move this one to ../po/ and "make merge" in po
messages:
	$(XGETTEXT) -C -ktranslate -x ../../kdelibs/po/kde.pot $(kdvi_SOURCES) && mv messages.po ../po/kdvi.pot

# just install datas here. Use install-exec-data for scripts and etc.
# the binary itself is already installed from automake
# use mkinstalldirs, not "install -d"
# don't install a list of file. Just one file per install.
# if you have more of them, create a subdirectory with an extra Makefile 
install-data-local: 
	$(mkinstalldirs) $(APPSDIR)
	$(INSTALL_DATA) kdvi.kdelnk $(APPSDIR)
	$(mkinstalldirs) $(kde_icondir)
	$(INSTALL_DATA) pix/kdvi.xpm $(kde_icondir)
	$(mkinstalldirs) $(kde_minidir)
	$(INSTALL_DATA) pix/mini-kdvi.xpm $(kde_minidir)/kdvi.xpm

# remove ALL you have installed in install-data-local or install-exec-local
uninstall-local:
	-rm -f $(APPSDIR)/Kdvi.kdelnk
	-rm -f $(kde_icondir)/kdvi.xpm
	-rm -f $(kde_minidir)/kdvi.xpm

distclean-local:
	(cd kpathsea && test -r config.status && $(MAKE) distclean)

# add a dependency for every moc file to be full portable

mdviwin.cpp: dviwin.h
	$(MOC) dviwin.h -o mdviwin.cpp

mkdvi.cpp: kdvi.h
	$(MOC) kdvi.h -o mkdvi.cpp

mmarklist.cpp: marklist.h
	$(MOC) marklist.h -o mmarklist.cpp

mprefs.cpp: prefs.h
	$(MOC) prefs.h -o mprefs.cpp

mprint.cpp: print.h
	$(MOC) print.h -o mprint.cpp

mprintData.cpp: printData.h
	$(MOC) printData.h -o mprintData.cpp

mprintSetup.cpp: printSetup.h
	$(MOC) printSetup.h -o mprintSetup.cpp

mprintSetupData.cpp: printSetupData.h
	$(MOC) printSetupData.h -o mprintSetupData.cpp

mpushbutton.cpp: pushbutton.h
	$(MOC) pushbutton.h -o mpushbutton.cpp

mscrbox.cpp : scrbox.h
	$(MOC) scrbox.h -o mscrbox.cpp


psheader.c: psheader.txt squeeze $(kpathsea)
	./squeeze psheader.txt $@

# this is somehow a dirty hack to force the automatic dependency checking
# to create the stuff in the kpathsea directory first. vf.c is the last
# source file in the list.
vf.c: $(kpathsea)

$(kpathsea):
	cd kpathsea && CC="$(CC)" && CFLAGS="$(CFLAGS)" && ./configure && $(MAKE)
