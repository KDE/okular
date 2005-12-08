SUBDIRS = conf .

INCLUDES = -I$(srcdir)/../.. -I$(top_builddir)/kpdf $(all_includes) 

METASOURCES = AUTO
KDE_CXXFLAGS=-Wall

bin_PROGRAMS = kpdflibgsasyncgenerator
kpdflibgsasyncgenerator_SOURCES = kpdflibgsasyncgenerator.cpp
kpdflibgsasyncgenerator_LDFLAGS = $(QT_LDFLAGS) $(all_libraries) -L$(LIB_GS_PATH)
kpdflibgsasyncgenerator_LDADD = -lqgs $(LIB_QT) $(LIB_KDECORE)
kde_module_LTLIBRARIES = libokularGenerator_ghostview.la

libokularGenerator_ghostview_la_LIBADD = $(top_builddir)/kpdf/generators/ghostview/conf/libgssettings.la \
	$(top_builddir)/kpdf/core/liboKularcore.la $(top_builddir)/kpdf/conf/liboKularconf.la -lqgs $(LIB_KDEUI) $(LIB_KDEPRINT) 
libokularGenerator_ghostview_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -L$(LIB_GS_PATH)
libokularGenerator_ghostview_la_SOURCES = interpreter_cmd.cpp internaldocument.cpp generator_ghostview.cpp gvlogwindow.cpp dscparse_adapter.cpp ps.c dscparse.cpp
#libokularGenerator_ghostview_la_METASOURCES = AUTO

KDE_OPTIONS = nofinal


kde_services_DATA = libokularGenerator_ghostview.desktop


