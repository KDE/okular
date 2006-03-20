kde_module_LTLIBRARIES = \
    libokularGenerator_poppler.la

INCLUDES = -I.. $(POPPLER_CFLAGS) $(all_includes) 

libokularGenerator_poppler_la_LIBADD = ../../core/liboKularcore.la ../../conf/liboKularconf.la \
				    $(POPPLER_LIBS) $(LIB_KDEPRINT) $(LIB_KDEUI) -lm
libokularGenerator_poppler_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
libokularGenerator_poppler_la_SOURCES = generator_pdf.cpp gp_outputdev.cpp pagetransition.cpp

KDE_OPTIONS = nofinal

METASOURCES = AUTO

kde_services_DATA = libokularGenerator_poppler.desktop

generator_pdf.lo: ../../conf/settings.h
