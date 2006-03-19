kde_module_LTLIBRARIES = \
    libokularGenerator_xpdf.la

INCLUDES = -I$(srcdir)/../.. -I$(top_builddir)/kpdf $(POPPLER_CFLAGS) $(all_includes) 

libokularGenerator_xpdf_la_LIBADD = $(top_builddir)/kpdf/core/liboKularcore.la $(top_builddir)/kpdf/conf/liboKularconf.la \
				    $(POPPLER_LIBS) $(LIB_KDEPRINT) $(LIB_KDEUI) -lm
libokularGenerator_xpdf_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
libokularGenerator_xpdf_la_SOURCES = generator_pdf.cpp gp_outputdev.cpp pagetransition.cpp

KDE_OPTIONS = nofinal

METASOURCES = AUTO

kde_services_DATA = libokularGenerator_xpdf.desktop

generator_pdf.lo: ../../conf/settings.h
