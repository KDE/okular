install_files( ${SERVICETYPES_INSTALL_DIR} FILES  okularGenerator.desktop )

#original Makefile.am contents follow:

#SUBDIRS = .
#
#INCLUDES = -I.. -I$(builddir)/../conf $(all_includes)
#
#METASOURCES = AUTO
#
#libokularcore_la_SOURCES = chooseenginedialog.ui generator.cpp area.cpp textpage.cpp page.cpp document.cpp link.cpp annotations.cpp \
#                         pagetransition.cpp
#libokularcore_la_LIBADD = $(KDE_LDFLAGS) $(LIB_KIO)
#
#kde_servicetypes_DATA = \
#    okularGenerator.desktop
#
#noinst_LTLIBRARIES = libokularcore.la
#
#document.lo: ../conf/settings.h
#page.lo: ../conf/settings.h
