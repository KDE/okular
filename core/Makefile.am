kde_module_LTLIBRARIES = \
    libokularGenerator_xpdf.la

INCLUDES = -I$(srcdir)/../.. -I$(srcdir)/../../xpdf -I$(srcdir)/../../xpdf/goo -I$(srcdir)/../../xpdf/splash -I$(top_builddir)/kpdf $(all_includes)

libokularGenerator_xpdf_la_LIBADD = $(top_builddir)/kpdf/core/libkpdfcore.la $(top_builddir)/kpdf/conf/libkpdfconf.la $(top_builddir)/kpdf/xpdf/xpdf/libxpdf.la -lkdeui -lkdeprint
libokularGenerator_xpdf_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
libokularGenerator_xpdf_la_SOURCES = generator_pdf.cpp gp_outputdev.cpp

KDE_OPTIONS = nofinal

kde_services_DATA = libokularGenerator_xpdf.desktop

generator_pdf.lo: ../../conf/settings.h
