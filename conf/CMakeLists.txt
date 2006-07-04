install(FILES okular.kcfg DESTINATION ${KCFG_INSTALL_DIR})

#original Makefile.am contents follow:

#INCLUDES = -I$(srcdir)/.. -I$(top_builddir)/kpdf $(all_includes)
#
#METASOURCES = AUTO
#
#libokularconf_la_SOURCES = dlggeneral.ui dlgperformance.ui dlgaccessibility.ui \
#                         dlgpresentation.ui \
#                         preferencesdialog.cpp settings.kcfgc
#
#noinst_LTLIBRARIES = libokularconf.la
#
#kde_kcfg_DATA = okular.kcfg
#
#preferencesdialog.lo: settings.h
