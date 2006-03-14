kde4_header()

add_subdirectory( conf ) 
add_subdirectory( core ) 
add_subdirectory( ui ) 
add_subdirectory( shell ) 

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )

macro_optional_find_package(KPDF)

########### next target ###############

set(kpdfpart_PART_SRCS part.cpp )

kde4_automoc(${kpdfpart_PART_SRCS})

kde4_add_dcop_skels(kpdfpart_PART_SRCS dcop.h )

kde4_add_plugin(kpdfpart WITH_PREFIX ${kpdfpart_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kpdfpart )

target_link_libraries(kpdfpart  ${QT_AND_KDECORE_LIBS} kpdfconf kpdfcore kpdfui kparts kdeprint kutils m )

install_targets(${LIB_INSTALL_DIR}/kde4 kpdfpart )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES kpdf_part.desktop )
install_files( ${DATA_INSTALL_DIR}/kpdfpart FILES part.rc )

kde4_install_icons( ${ICON_INSTALL_DIR}  hicolor )

kde4_footer()



#original Makefile.am contents follow:

#SUBDIRS = conf core ui shell
#
#INCLUDES = -I$(top_builddir)/kpdf $(all_includes) $(POPPLER_CFLAGS)
#
#METASOURCES = AUTO
#
#messages: rc.cpp
#	$(EXTRACTRC) `find . -name "*.rc" -o -name "*.ui"` >> rc.cpp
#	$(XGETTEXT) `find . -name "*.cpp" -o -name "*.cc" -o -name "*.h"` -o $(podir)/kpdf.pot
#
#KDE_ICON = kpdf
#
##########################################################################
## KPART SECTION
##########################################################################
#kde_module_LTLIBRARIES = libkpdfpart.la
#
#libkpdfpart_la_SOURCES = dcop.skel error.cpp part.cpp
#libkpdfpart_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#libkpdfpart_la_LIBADD  = $(POPPLER_LIBS) conf/libkpdfconf.la \
#                         core/libkpdfcore.la ui/libkpdfui.la $(LIB_KPARTS) \
#                         $(LIB_KFILE) $(LIB_KDEPRINT) $(LIB_KUTILS) -lm
#
#partdesktopdir   = $(kde_servicesdir)
#partdesktop_DATA = kpdf_part.desktop
#
#partrcdir   = $(kde_datadir)/kpdfpart
#partrc_DATA = part.rc
#
#part.lo: conf/settings.h
