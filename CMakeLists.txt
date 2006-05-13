project(oKular)

add_subdirectory( conf )
add_subdirectory( core )
add_subdirectory( ui )
add_subdirectory( shell )
add_subdirectory( generators )

include_directories(
   ${CMAKE_SOURCE_DIR}/okular/ui/painter_agg2/
)


########### next target ###############

set(oKularconf_SRCS
   ${CMAKE_SOURCE_DIR}/okular/conf/preferencesdialog.cpp
   ${CMAKE_SOURCE_DIR}/okular/conf/dlgaccessibility.cpp
   ${CMAKE_SOURCE_DIR}/okular/conf/dlggeneral.cpp
   ${CMAKE_SOURCE_DIR}/okular/conf/dlgperformance.cpp
   ${CMAKE_SOURCE_DIR}/okular/conf/dlgpresentation.cpp
)

kde4_add_ui_files(oKularconf_SRCS
   ${CMAKE_SOURCE_DIR}/okular/conf/dlgaccessibilitybase.ui
   ${CMAKE_SOURCE_DIR}/okular/conf/dlggeneralbase.ui
   ${CMAKE_SOURCE_DIR}/okular/conf/dlgperformancebase.ui
   ${CMAKE_SOURCE_DIR}/okular/conf/dlgpresentationbase.ui
)

set(oKularcore_SRCS
   ${CMAKE_SOURCE_DIR}/okular/core/annotations.cpp
   ${CMAKE_SOURCE_DIR}/okular/core/area.cpp
   ${CMAKE_SOURCE_DIR}/okular/core/chooseenginedialog.cpp
   ${CMAKE_SOURCE_DIR}/okular/core/document.cpp
   ${CMAKE_SOURCE_DIR}/okular/core/generator.cpp
   ${CMAKE_SOURCE_DIR}/okular/core/link.cpp
   ${CMAKE_SOURCE_DIR}/okular/core/page.cpp
   ${CMAKE_SOURCE_DIR}/okular/core/pagetransition.cpp
   ${CMAKE_SOURCE_DIR}/okular/core/textpage.cpp
)

kde4_add_ui_files(oKularcore_SRCS
   ${CMAKE_SOURCE_DIR}/okular/core/chooseenginewidget.ui
)

set(oKularui_SRCS
   ${CMAKE_SOURCE_DIR}/okular/ui/minibar.cpp
   ${CMAKE_SOURCE_DIR}/okular/ui/newstuff.cpp
   ${CMAKE_SOURCE_DIR}/okular/ui/pagepainter.cpp
   ${CMAKE_SOURCE_DIR}/okular/ui/pageviewannotator.cpp
   ${CMAKE_SOURCE_DIR}/okular/ui/pageview.cpp
   ${CMAKE_SOURCE_DIR}/okular/ui/pageviewutils.cpp
   ${CMAKE_SOURCE_DIR}/okular/ui/presentationwidget.cpp
   ${CMAKE_SOURCE_DIR}/okular/ui/propertiesdialog.cpp
   ${CMAKE_SOURCE_DIR}/okular/ui/searchwidget.cpp
   ${CMAKE_SOURCE_DIR}/okular/ui/side_reviews.cpp
   ${CMAKE_SOURCE_DIR}/okular/ui/thumbnaillist.cpp
   ${CMAKE_SOURCE_DIR}/okular/ui/toc.cpp
)

set(oKularagg2_SRCS
   ${CMAKE_SOURCE_DIR}/okular/ui/painter_agg2/agg_bezier_arc.cpp
   ${CMAKE_SOURCE_DIR}/okular/ui/painter_agg2/agg_path_storage.cpp
   ${CMAKE_SOURCE_DIR}/okular/ui/painter_agg2/agg_rasterizer_scanline_aa.cpp
   ${CMAKE_SOURCE_DIR}/okular/ui/painter_agg2/agg_trans_affine.cpp
   ${CMAKE_SOURCE_DIR}/okular/ui/painter_agg2/agg_vcgen_stroke.cpp
)
kde4_add_kcfg_files(oKularconf_SRCS ${CMAKE_SOURCE_DIR}/okular/conf/settings.kcfgc )

set(oKularcore_SRCS ${oKularconf_SRCS} ${oKularcore_SRCS} )

kde4_automoc(${oKularcore_SRCS})

kde4_add_library(oKularcore SHARED ${oKularcore_SRCS})

target_link_libraries(oKularcore  ${KDE4_KDECORE_LIBS} ${KDE4_KPARTS_LIBS} m )

install_targets(${LIB_INSTALL_DIR} oKularcore )

set(oKularpart_PART_SRCS part.cpp ${oKularui_SRCS} ${oKularagg2_SRCS} )

kde4_automoc(${oKularpart_PART_SRCS})

kde4_add_dcop_skels(oKularpart_PART_SRCS dcop.h )

kde4_add_plugin(oKularpart WITH_PREFIX ${oKularpart_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} oKularpart )

target_link_libraries(oKularpart  oKularcore ${KDE4_KDECORE_LIBS} ${KDE4_KPARTS_LIBS} ${KDE4_KDEPRINT_LIBS} ${KDE4_KUTILS_LIBS} ${KDE4_KNEWSTUFF_LIBS} ${KDE4_KHTML_LIBS} m ${X11_Xrender_LIB} )

install_targets(${PLUGIN_INSTALL_DIR} oKularpart )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES  oKular_part.desktop )
install_files( ${DATA_INSTALL_DIR}/oKularpart FILES  part.rc )

kde4_install_icons( ${ICON_INSTALL_DIR}  hicolor )





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
