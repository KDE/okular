kde_module_LTLIBRARIES = \
    libokularGenerator_kimgio.la

INCLUDES = -I$(srcdir)/../../ $(all_includes)

libokularGenerator_kimgio_la_LIBADD = $(top_builddir)/kpdf/core/liboKularcore.la \
	$(top_builddir)/kpdf/conf/liboKularconf.la $(LIB_KDEUI)
libokularGenerator_kimgio_la_LDFLAGS = -module -avoid-version  $(KDE_PLUGIN)  $(all_libraries)
libokularGenerator_kimgio_la_SOURCES = generator_kimgio.cpp

METASOURCES = AUTO 

kde_services_DATA = libokularGenerator_kimgio.desktop
