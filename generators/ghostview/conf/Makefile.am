noinst_LTLIBRARIES = libgssettings.la

INCLUDES = -I$(srcdir)/../.. -I$(top_builddir)/kpdf $(all_includes)

libgssettings_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module -lqt-mt
libgssettings_la_LIBADD = $(LIB_KDEUI) $(LIB_KIO)
libgssettings_la_SOURCES = gssettingswidget.ui gssettings.kcfgc

METASOURCES = AUTO

kde_kcfg_DATA = gssettings.kcfg

