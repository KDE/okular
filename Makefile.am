SUBDIRS = xpdf conf core ui

INCLUDES = -I$(srcdir)/xpdf -I$(srcdir)/xpdf/goo $(all_includes) $(FREETYPE_CFLAGS)

METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kpdf.pot

KDE_ICON = kpdf

# #########################################################################
# # APPLICATION SECTION
# #########################################################################
bin_PROGRAMS = kpdf

kpdf_SOURCES = main.cpp shell.cpp
kpdf_LDFLAGS = $(KDE_RPATH) $(all_libraries)
kpdf_LDADD   = $(LIB_KPARTS)

EXTRA_DIST = kpdf.desktop
xdg_apps_DATA = kpdf.desktop

shellrcdir   = $(kde_datadir)/kpdf
shellrc_DATA = shell.rc

#########################################################################
# KPART SECTION
#########################################################################
kde_module_LTLIBRARIES = libkpdfpart.la

libkpdfpart_la_SOURCES = dcop.skel error.cpp part.cpp
libkpdfpart_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
libkpdfpart_la_LIBADD  = xpdf/xpdf/libxpdf.la conf/libkpdfconf.la \
                         core/libkpdfcore.la ui/libkpdfui.la $(LIB_KPARTS) \
                         $(LIB_KFILE) $(LIB_KDEPRINT) $(LIB_KUTILS) -lm

partdesktopdir   = $(kde_servicesdir)
partdesktop_DATA = kpdf_part.desktop

partrcdir   = $(kde_datadir)/kpdfpart
partrc_DATA = part.rc
