SUBDIRS = .

INCLUDES = -I$(srcdir)/.. -I$(top_builddir)/kpdf $(all_includes)

METASOURCES = AUTO

liboKularcore_la_SOURCES = chooseenginedialog.ui area.cpp textpage.cpp page.cpp document.cpp link.cpp annotations.cpp \
                         pagetransition.cpp
liboKularcore_la_LIBADD = -lkio 

kde_servicetypes_DATA = \
    oKularGenerator.desktop

noinst_LTLIBRARIES = liboKularcore.la

document.lo: ../conf/settings.h
page.lo: ../conf/settings.h
