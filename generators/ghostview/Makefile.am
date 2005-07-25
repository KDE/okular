#SUBDIRS = conf
kde_module_LTLIBRARIES = \
    libokularGenerator_ghostview.la

INCLUDES = -I$(srcdir)/../.. -I$(top_builddir)/kpdf $(all_includes)

libokularGenerator_ghostview_la_LIBADD = $(top_builddir)/kpdf/core/libkpdfcore.la $(top_builddir)/kpdf/conf/libkpdfconf.la -lgs -lkdeui -lkdeprint
libokularGenerator_ghostview_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
libokularGenerator_ghostview_la_SOURCES = interpreter_cmd.cpp generator_ghostview.cpp dscparse_adapter.cpp dscparse.cpp

KDE_OPTIONS = nofinal

kde_services_DATA = libokularGenerator_ghostview.desktop

# generator_pdf.lo: ../../conf/settings.h
