INCLUDES = -I$(srcdir)/../.. -I$(srcdir)/../../xpdf -I$(srcdir)/../../xpdf/goo -I$(srcdir)/../../xpdf/splash -I$(top_builddir)/kpdf $(all_includes)

libgeneratorpdf_la_LDFLAGS = $(all_libraries)
libgeneratorpdf_la_SOURCES = generator_pdf.cpp gp_outputdev.cpp

noinst_LTLIBRARIES = libgeneratorpdf.la

KDE_OPTIONS = nofinal

generator_pdf.lo: ../../conf/settings.h
