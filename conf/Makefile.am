INCLUDES = -I$(srcdir)/.. -I$(top_builddir)/kpdf $(all_includes)

METASOURCES = AUTO

libkpdfconf_la_SOURCES = dlggeneral.ui dlgperformance.ui dlgaccessibility.ui \
                         dlgpresentation.ui \
                         preferencesdialog.cpp settings.kcfgc

noinst_LTLIBRARIES = libkpdfconf.la

kde_kcfg_DATA = kpdf.kcfg

preferencesdialog.lo: settings.h
