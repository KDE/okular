# set the include path for X, qt and KDE
INCLUDES = -I$(srcdir)/.. -I$(srcdir)/../splash -I$(srcdir)/../goo -I$(srcdir)/../xpdf $(all_includes) $(FREETYPE_CFLAGS)

# these are the headers for your project
noinst_HEADERS = kpdf_shell.h kpdf_part.h kpdf_pagewidget.h QOutputDevPixmap.h QOutputDev.h

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp 
	$(XGETTEXT) *.cpp -o $(podir)/kpdf.pot

# TODO: fix the code!

KDE_ICON = kpdf

# this Makefile creates both a KPart application and a KPart
#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = kpdf

# the application source, library search path, and link libraries
kpdf_SOURCES = main.cpp kpdf_shell.cpp 
kpdf_LDFLAGS = $(KDE_RPATH) $(all_libraries)
kpdf_LDADD   = $(LIB_KPARTS) 
kpdf_COMPILE_FIRST = part.h

EXTRA_DIST = kpdf.desktop
xdg_apps_DATA = kpdf.desktop

# this is where the shell's XML-GUI resource file goes
shellrcdir   = $(kde_datadir)/kpdf
shellrc_DATA = kpdf_shell.rc

#########################################################################
# KPART SECTION
#########################################################################
kde_module_LTLIBRARIES = libkpdfpart.la

# the Part's source, library search path, and link libraries
libkpdfpart_la_SOURCES = QOutputDev.cpp QOutputDevPixmap.cpp kpdf_part.cpp kpdf_pagewidget.cc part.cpp thumbnail.cpp thumbnaillist.cpp kpdf_error.cpp thumbnailgenerator.cpp toc.cpp
libkpdfpart_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
libkpdfpart_la_LIBADD  = ../xpdf/libxpdf.la $(LIB_KPARTS) $(LIB_KFILE) $(LIB_KDEPRINT) $(LIB_KUTILS) -lm

# this is where the desktop file will go 
partdesktopdir   = $(kde_servicesdir)
partdesktop_DATA = kpdf_part.desktop

# this is where the part's XML-GUI resource file goes
partrcdir   = $(kde_datadir)/kpdfpart
partrc_DATA = kpdf_part.rc

