SUBDIRS = conf core ui shell

INCLUDES = -I$(top_builddir)/kpdf $(all_includes) $(POPPLER_CFLAGS)

METASOURCES = AUTO


KDE_ICON = kpdf

#########################################################################
# KPART SECTION
#########################################################################
kde_module_LTLIBRARIES = libkpdfpart.la

libkpdfpart_la_SOURCES = dcop.skel  part.cpp
libkpdfpart_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
libkpdfpart_la_LIBADD  = $(POPPLER_LIBS) conf/libkpdfconf.la \
                         core/libkpdfcore.la ui/libkpdfui.la $(LIB_KPARTS) \
                         $(LIB_KFILE) $(LIB_KDEPRINT) $(LIB_KUTILS) -lm

partdesktopdir   = $(kde_servicesdir)
partdesktop_DATA = kpdf_part.desktop

partrcdir   = $(kde_datadir)/kpdfpart
partrc_DATA = part.rc

part.lo: conf/settings.h
