SUBDIRS = conf .

INCLUDES = -I$(srcdir)/../.. -I$(top_builddir)/kpdf $(all_includes) 

METASOURCES = AUTO
KDE_CXXFLAGS=-Wall

noinst_LTLIBRARIES = libgsinterpreter.la
libgsinterpreter_la_LDFLAGS = $(all_libraries) -no-undefined $(GSLIBDIR)
libgsinterpreter_la_LIBADD = $(LIB_QT) $(LIB_KDECORE) conf/libgssettings.la -lgs
libgsinterpreter_la_SOURCES = interpreter_lib.cpp 

bin_PROGRAMS = kpdflibgsasyncgenerator
kpdflibgsasyncgenerator_SOURCES = kpdflibgsasyncgenerator.cpp
kpdflibgsasyncgenerator_LDFLAGS = $(LIB_QT)
kpdflibgsasyncgenerator_LDADD = libgsinterpreter.la

kde_module_LTLIBRARIES = libokularGenerator_ghostview.la

libokularGenerator_ghostview_la_LIBADD = $(top_builddir)/kpdf/core/libkpdfcore.la libgsinterpreter.la $(top_builddir)/kpdf/conf/libkpdfconf.la -lgs -lkdeui -lkdeprint 
libokularGenerator_ghostview_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
libokularGenerator_ghostview_la_SOURCES = interpreter_cmd.cpp internaldocument.cpp generator_ghostview.cpp gvlogwindow.cpp dscparse_adapter.cpp ps.c dscparse.cpp
#libokularGenerator_ghostview_la_METASOURCES = AUTO

KDE_OPTIONS = nofinal


kde_services_DATA = libokularGenerator_ghostview.desktop


