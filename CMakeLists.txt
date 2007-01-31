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

add_subdirectory( ui )
add_subdirectory( shell )
add_subdirectory( generators )

include_directories(
   ${CMAKE_SOURCE_DIR}
   ${CMAKE_SOURCE_DIR}/okular/ui/painter_agg2/
)


# okularcore

set(okularcore_SRCS
   core/annotations.cpp
   core/area.cpp
   core/bookmarkmanager.cpp
   core/chooseenginedialog.cpp
   core/document.cpp
   core/generator.cpp
   core/link.cpp
   core/misc.cpp
   core/observer.cpp
   core/page.cpp
   core/pagesize.cpp
   core/pagetransition.cpp
   core/rotationjob.cpp
   core/sound.cpp
   core/sourcereference.cpp
   core/textdocumentgenerator.cpp
   core/textpage.cpp
   core/utils.cpp
)

install( FILES
           core/annotations.h
           core/area.h
           core/document.h
           core/generator.h
           core/link.h
           core/observer.h
           core/page.h
           core/pagesize.h
           core/pagetransition.h
           core/sound.h
           core/sourcereference.h
           core/textdocumentgenerator.h
           core/textpage.h
           core/utils.h
         DESTINATION ${INCLUDE_INSTALL_DIR}/okular/core )

install( FILES
           interfaces/configinterface.h
           interfaces/guiinterface.h
           interfaces/printinterface.h
         DESTINATION ${INCLUDE_INSTALL_DIR}/okular/interfaces )

kde4_add_ui_files(okularcore_SRCS
   core/chooseenginewidget.ui
)

kde4_add_kcfg_files(okularcore_SRCS conf/settings.kcfgc )

kde4_automoc(${okularcore_SRCS})

kde4_add_library(okularcore SHARED ${okularcore_SRCS})

# Special handling for linking okularcore on OSX/Apple
IF(APPLE)
    SET(OKULAR_IOKIT "-framework IOKit" CACHE STRING "Apple IOKit framework")
ENDIF(APPLE)

target_link_libraries(okularcore  ${OKULAR_IOKIT} ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBRARY} ${KDE4_KDEPRINT_LIBS} m )

install(TARGETS okularcore DESTINATION ${LIB_INSTALL_DIR} )

install(FILES conf/okular.kcfg DESTINATION ${KCFG_INSTALL_DIR})
install(FILES core/okularGenerator.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})

# okularpart

set(okularpart_SRCS
   part.cpp
   conf/preferencesdialog.cpp
   conf/dlgaccessibility.cpp
   conf/dlgdebug.cpp
   conf/dlggeneral.cpp
   conf/dlgidentity.cpp
   conf/dlgperformance.cpp
   conf/dlgpresentation.cpp
   ui/embeddedfilesdialog.cpp
   ui/annotwindow.cpp
   ui/annotationguiutils.cpp
   ui/annotationpopup.cpp
   ui/annotationpropertiesdialog.cpp
   ui/annotationtools.cpp
   ui/annotationwidgets.cpp
   ui/bookmarklist.cpp
   ui/minibar.cpp
   ui/newstuff.cpp
   ui/pageitemdelegate.cpp
   ui/pagepainter.cpp
   ui/pagesizelabel.cpp
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

kde4_add_ui_files(okularpart_SRCS
   conf/dlgaccessibilitybase.ui
   conf/dlggeneralbase.ui
   conf/dlgidentitybase.ui
   conf/dlgperformancebase.ui
   conf/dlgpresentationbase.ui
)

kde4_automoc(${okularpart_SRCS})

kde4_add_plugin(okularpart WITH_PREFIX ${okularpart_SRCS})

target_link_libraries(okularpart  okularcore  ${KDE4_KPARTS_LIBS} ${KDE4_KDEPRINT_LIBS} ${KDE4_KNEWSTUFF_LIBS} ${KDE4_KHTML_LIBS} m )
if ( X11_Xrender_FOUND )
   target_link_libraries( okularpart ${X11_Xrender_LIB} )
endif ( X11_Xrender_FOUND )

install(TARGETS okularpart DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES okular_part.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES part.rc DESTINATION ${DATA_INSTALL_DIR}/okularpart )

kde4_install_icons( ${ICON_INSTALL_DIR}   )

