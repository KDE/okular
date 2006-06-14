project(okular)

option(
   OKULAR_FORCE_DRM
   "Forces okular to check for DRM to decide if you can copy/print protected pdf. (default=no)"
   OFF
)
if(OKULAR_FORCE_DRM STREQUAL "ON")
   set(_OKULAR_FORCE_DRM 1)
else(OKULAR_FORCE_DRM STREQUAL "ON")
   set(_OKULAR_FORCE_DRM 0)
endif(OKULAR_FORCE_DRM STREQUAL "ON")

configure_file(
   ${CMAKE_CURRENT_SOURCE_DIR}/config-okular.h.cmake
   ${CMAKE_CURRENT_BINARY_DIR}/config-okular.h
)

add_subdirectory( conf )
add_subdirectory( core )
add_subdirectory( ui )
add_subdirectory( shell )
add_subdirectory( generators )

include_directories(
   ${CMAKE_SOURCE_DIR}/okular/ui/painter_agg2/
)


# okularcore

set(okularcore_SRCS
   conf/preferencesdialog.cpp
   conf/dlgaccessibility.cpp
   conf/dlggeneral.cpp
   conf/dlgperformance.cpp
   conf/dlgpresentation.cpp
   core/annotations.cpp
   core/area.cpp
   core/chooseenginedialog.cpp
   core/document.cpp
   core/generator.cpp
   core/link.cpp
   core/page.cpp
   core/pagetransition.cpp
   core/textpage.cpp
)

kde4_add_ui_files(okularcore_SRCS
   conf/dlgaccessibilitybase.ui
   conf/dlggeneralbase.ui
   conf/dlgperformancebase.ui
   conf/dlgpresentationbase.ui
   core/chooseenginewidget.ui
)

kde4_add_kcfg_files(okularcore_SRCS conf/settings.kcfgc )

kde4_automoc(${okularcore_SRCS})

kde4_add_library(okularcore SHARED ${okularcore_SRCS})

target_link_libraries(okularcore  ${KDE4_KDECORE_LIBS} ${KDE4_KPARTS_LIBS} m )

install_targets(${LIB_INSTALL_DIR} okularcore )

# okularpart

set(okularpart_SRCS
   part.cpp
   ui/embeddedfilesdialog.cpp
   ui/minibar.cpp
   ui/newstuff.cpp
   ui/pagepainter.cpp
   ui/pageviewannotator.cpp
   ui/pageview.cpp
   ui/pageviewutils.cpp
   ui/presentationwidget.cpp
   ui/propertiesdialog.cpp
   ui/searchwidget.cpp
   ui/side_reviews.cpp
   ui/thumbnaillist.cpp
   ui/toc.cpp
   ui/painter_agg2/agg_bezier_arc.cpp
   ui/painter_agg2/agg_path_storage.cpp
   ui/painter_agg2/agg_rasterizer_scanline_aa.cpp
   ui/painter_agg2/agg_trans_affine.cpp
   ui/painter_agg2/agg_vcgen_stroke.cpp
)

kde4_automoc(${okularpart_SRCS})

kde4_add_plugin(okularpart WITH_PREFIX ${okularpart_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} okularpart )

target_link_libraries(okularpart  okularcore ${KDE4_KDECORE_LIBS} ${KDE4_KPARTS_LIBS} ${KDE4_KDEPRINT_LIBS} ${KDE4_KNEWSTUFF_LIBS} ${KDE4_KHTML_LIBS} m ${X11_Xrender_LIB} )

install_targets(${PLUGIN_INSTALL_DIR} okularpart )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES  okular_part.desktop )
install_files( ${DATA_INSTALL_DIR}/okularpart FILES  part.rc )

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
#	$(XGETTEXT) `find . -name "*.cpp" -o -name "*.cc" -o -name "*.h"` -o $(podir)/okular.pot
#
#KDE_ICON = okular
#
##########################################################################
## KPART SECTION
##########################################################################
#kde_module_LTLIBRARIES = libokularpart.la
#
#libokularpart_la_SOURCES = dcop.skel part.cpp
#libokularpart_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#libokularpart_la_LIBADD  = conf/libokularconf.la core/libokularcore.la \
#                         ui/libokularui.la ui/painter_agg2/libagg2.la $(LIB_KPARTS) \
#                         $(LIB_KFILE) $(LIB_KDEPRINT) $(LIB_KUTILS) $(LIB_KNEWSTUFF) \
#                         $(LIB_KHTML) -lm 
#
#partdesktopdir   = $(kde_servicesdir)
#partdesktop_DATA = okular_part.desktop
#
#partrcdir   = $(kde_datadir)/okularpart
#partrc_DATA = part.rc
#
#part.lo: conf/settings.h
#
#include $(top_srcdir)/admin/Doxyfile.am
#
