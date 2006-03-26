kde4_header()

add_subdirectory( conf ) 
add_subdirectory( core ) 
add_subdirectory( ui ) 
add_subdirectory( shell ) 
add_subdirectory( generators ) 

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(oKularpart_PART_SRCS part.cpp )

kde4_automoc(${oKularpart_PART_SRCS})

kde4_add_dcop_skels(oKularpart_PART_SRCS dcop.h )

kde4_add_plugin(oKularpart WITH_PREFIX ${oKularpart_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} oKularpart )

target_link_libraries(oKularpart  ${KDE4_KDECORE_LIBS} oKularconf oKularcore oKularui agg2 ${KDE4_KPARTS_LIBS} ${KDE4_KDEPRINT_LIBS} ${KDE4_KUTILS_LIBS} ${KDE4_KNEWSTUFF_LIBS} ${KDE4_KHTML_LIBS} m )

install_targets(${LIB_INSTALL_DIR}/kde4 oKularpart )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES  oKular_part.desktop )
install_files( ${DATA_INSTALL_DIR}/oKularpart FILES  part.rc )

kde4_install_icons( ${ICON_INSTALL_DIR}  hicolor )

kde4_create_doxygen_docs(  )

kde4_footer()



#original Makefile.am contents follow:

#SUBDIRS = conf core ui shell generators
# 
#INCLUDES = -Iconf $(all_includes) $(FREETYPE_CFLAGS)
#
#METASOURCES = AUTO
#
#messages: rc.cpp
#	$(EXTRACTRC) `find . -name "*.rc" -o -name "*.ui"` >> rc.cpp
#	$(XGETTEXT) `find . -name "*.cpp" -o -name "*.cc" -o -name "*.h"` -o $(podir)/oKular.pot
#
#KDE_ICON = oKular
#
##########################################################################
## KPART SECTION
##########################################################################
#kde_module_LTLIBRARIES = liboKularpart.la
#
#liboKularpart_la_SOURCES = dcop.skel part.cpp
#liboKularpart_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#liboKularpart_la_LIBADD  = conf/liboKularconf.la core/liboKularcore.la \
#                         ui/liboKularui.la ui/painter_agg2/libagg2.la $(LIB_KPARTS) \
#                         $(LIB_KFILE) $(LIB_KDEPRINT) $(LIB_KUTILS) $(LIB_KNEWSTUFF) \
#                         $(LIB_KHTML) -lm 
#
#partdesktopdir   = $(kde_servicesdir)
#partdesktop_DATA = oKular_part.desktop
#
#partrcdir   = $(kde_datadir)/oKularpart
#partrc_DATA = part.rc
#
#part.lo: conf/settings.h
#
#include $(top_srcdir)/admin/Doxyfile.am
#
