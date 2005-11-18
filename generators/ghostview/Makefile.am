SUBDIRS = conf .

INCLUDES = -I$(srcdir)/../.. -I$(top_builddir)/kpdf $(all_includes) 

METASOURCES = AUTO
KDE_CXXFLAGS=-Wall

bin_PROGRAMS = kpdflibgsasyncgenerator
kpdflibgsasyncgenerator_SOURCES = kpdflibgsasyncgenerator.cpp
kpdflibgsasyncgenerator_LDFLAGS = $(LIB_QT) -lkgs

kde_module_LTLIBRARIES = libokularGenerator_ghostview.la

libokularGenerator_ghostview_la_LIBADD = $(top_builddir)/kpdf/core/liboKularcore.la $(top_builddir)/kpdf/conf/liboKularconf.la -lkgs -lkdeui -lkdeprint 
libokularGenerator_ghostview_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -L$(LIB_GS_PATH)
libokularGenerator_ghostview_la_SOURCES = interpreter_cmd.cpp internaldocument.cpp generator_ghostview.cpp gvlogwindow.cpp dscparse_adapter.cpp ps.c dscparse.cpp
#libokularGenerator_ghostview_la_METASOURCES = AUTO

KDE_OPTIONS = nofinal


kde_services_DATA = libokularGenerator_ghostview.desktop


