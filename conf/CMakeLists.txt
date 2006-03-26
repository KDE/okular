kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############


########### install files ###############

install_files( ${KCFG_INSTALL_DIR} FILES  oKular.kcfg )

kde4_footer()



#original Makefile.am contents follow:

#INCLUDES = -I$(srcdir)/.. -I$(top_builddir)/kpdf $(all_includes)
#
#METASOURCES = AUTO
#
#liboKularconf_la_SOURCES = dlggeneral.ui dlgperformance.ui dlgaccessibility.ui \
#                         dlgpresentation.ui \
#                         preferencesdialog.cpp settings.kcfgc
#
#noinst_LTLIBRARIES = liboKularconf.la
#
#kde_kcfg_DATA = oKular.kcfg
#
#preferencesdialog.lo: settings.h
