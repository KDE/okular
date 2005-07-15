SUBDIRS = . generator_pdf generator_kimgio

INCLUDES = -I$(srcdir)/generator_pdf -I$(srcdir)/.. -I$(srcdir)/../xpdf -I$(srcdir)/../xpdf/goo -I$(top_builddir)/kpdf $(all_includes)

METASOURCES = AUTO

libkpdfcore_la_SOURCES = area.cpp textpage.cpp page.cpp document.cpp link.cpp annotations.cpp \
                         pagetransition.cpp
libkpdfcore_la_LIBADD = -lkio 

kde_servicetypes_DATA = \
    oKularGenerator.desktop

noinst_LTLIBRARIES = libkpdfcore.la

document.lo: ../conf/settings.h
page.lo: ../conf/settings.h
