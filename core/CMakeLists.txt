kde4_header()

install_files( ${SERVICETYPES_INSTALL_DIR} FILES  oKularGenerator.desktop )

kde4_footer()

#original Makefile.am contents follow:

#SUBDIRS = .
#
#INCLUDES = -I.. -I$(builddir)/../conf $(all_includes)
#
#METASOURCES = AUTO
#
#liboKularcore_la_SOURCES = chooseenginedialog.ui generator.cpp area.cpp textpage.cpp page.cpp document.cpp link.cpp annotations.cpp \
#                         pagetransition.cpp
#liboKularcore_la_LIBADD = $(KDE_LDFLAGS) $(LIB_KIO)
#
#kde_servicetypes_DATA = \
#    oKularGenerator.desktop
#
#noinst_LTLIBRARIES = liboKularcore.la
#
#document.lo: ../conf/settings.h
#page.lo: ../conf/settings.h
