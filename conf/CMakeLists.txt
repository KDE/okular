kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############


########### install files ###############

install_files( ${KCFG_INSTALL_DIR} FILES kpdf.kcfg )

kde4_footer()



#original Makefile.am contents follow:

#INCLUDES = -I$(srcdir)/.. -I$(top_builddir)/kpdf $(all_includes)
#
#METASOURCES = AUTO
#
#libkpdfconf_la_SOURCES = dlggeneral.ui dlgperformance.ui dlgaccessibility.ui \
#                         dlgpresentation.ui \
#                         preferencesdialog.cpp settings.kcfgc
#
#noinst_LTLIBRARIES = libkpdfconf.la
#
#kde_kcfg_DATA = kpdf.kcfg
#
#preferencesdialog.lo: settings.h
