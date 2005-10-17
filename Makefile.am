SUBDIRS = conf core ui shell generators

INCLUDES =  -I$(top_builddir)/kpdf $(all_includes) $(FREETYPE_CFLAGS)

METASOURCES = AUTO

messages: rc.cpp
	$(EXTRACTRC) `find . -name "*.rc" -o -name "*.ui"` >> rc.cpp
	$(XGETTEXT) `find . -name "*.cpp" -o -name "*.cc" -o -name "*.h"` -o $(podir)/kpdf.pot

KDE_ICON = oKular

#########################################################################
# KPART SECTION
#########################################################################
kde_module_LTLIBRARIES = liboKularpart.la

liboKularpart_la_SOURCES = dcop.skel part.cpp
liboKularpart_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
liboKularpart_la_LIBADD  = conf/liboKularconf.la core/liboKularcore.la \
                         ui/libkpdfui.la ui/painter_agg2/libagg2.la $(LIB_KPARTS) \
                         $(LIB_KFILE) $(LIB_KDEPRINT) $(LIB_KUTILS) $(LIB_KNEWSTUFF) \
                         $(LIB_KHTML) -lm

partdesktopdir   = $(kde_servicesdir)
partdesktop_DATA = oKular_part.desktop

partrcdir   = $(kde_datadir)/oKularpart
partrc_DATA = part.rc

part.lo: conf/settings.h

include $(top_srcdir)/admin/Doxyfile.am

