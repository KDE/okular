INCLUDES = -I$(srcdir)/xpdf -I$(srcdir)/xpdf/goo -I$(top_builddir)/kpdf $(all_includes) $(FREETYPE_CFLAGS)

METASOURCES = AUTO

bin_PROGRAMS = kpdf

kpdf_SOURCES = main.cpp shell.cpp
kpdf_LDFLAGS = $(KDE_RPATH) $(all_libraries)
kpdf_LDADD   = $(LIB_KPARTS)

EXTRA_DIST = kpdf.desktop
xdg_apps_DATA = kpdf.desktop

shellrcdir   = $(kde_datadir)/kpdf
shellrc_DATA = shell.rc
