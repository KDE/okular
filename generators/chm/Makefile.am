SUBDIRS = lib . kio-msits 

#bin_PROGRAMS = helper
#helper_SOURCES = helper.cpp 
#helper_LDFLAGS = $(LIB_QT) 
#helper_LDADD = $(top_builddir)/kpdf/core/libkpdfcore.la $(top_builddir)/kpdf/conf/libkpdfconf.la lib/libchmfile.la

kde_module_LTLIBRARIES = \
  libokularGenerator_chmlib.la

INCLUDES = -I../.. $(all_includes)

libokularGenerator_chmlib_la_LIBADD = ../../core/liboKularcore.la \
	../../conf/liboKularconf.la \
	lib/libchmfile.la \
	$(LIB_KDEPRINT) $(LIB_KDEUI) $(LIB_KHTML) -lchm
libokularGenerator_chmlib_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
libokularGenerator_chmlib_la_SOURCES = generator_t.cpp
METASOURCES = AUTO
KDE_OPTIONS = nofinal

kde_services_DATA = libokularGenerator_chmlib.desktop

