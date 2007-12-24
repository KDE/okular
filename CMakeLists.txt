project(okular)

add_subdirectory( ui )
add_subdirectory( shell )
add_subdirectory( generators )

include(OkularConfigureChecks.cmake)

if(NOT WIN32)
 set(MATH_LIB m)
else(NOT WIN32)
 set(MATH_LIB)
endif(NOT WIN32)
include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${BLITZ_INCLUDES}
)


# okularcore

set(okularcore_SRCS
   core/action.cpp
   core/annotations.cpp
   core/area.cpp
   core/audioplayer.cpp
   core/bookmarkmanager.cpp
   core/chooseenginedialog.cpp
   core/document.cpp
   core/fontinfo.cpp
   core/form.cpp
   core/generator.cpp
   core/generator_p.cpp
   core/misc.cpp
   core/observer.cpp
   core/page.cpp
   core/pagecontroller.cpp
   core/pagesize.cpp
   core/pagetransition.cpp
   core/rotationjob.cpp
   core/sound.cpp
   core/sourcereference.cpp
   core/textdocumentgenerator.cpp
   core/textpage.cpp
   core/utils.cpp
   core/fileprinter.cpp
)

install( FILES
           core/action.h
           core/annotations.h
           core/area.h
           core/document.h
           core/fontinfo.h
           core/form.h
           core/generator.h
           core/global.h
           core/observer.h
           core/okular_export.h
           core/page.h
           core/pagesize.h
           core/pagetransition.h
           core/sound.h
           core/sourcereference.h
           core/textdocumentgenerator.h
           core/textpage.h
           core/utils.h
           core/fileprinter.h
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


kde4_add_library(okularcore SHARED ${okularcore_SRCS})

# Special handling for linking okularcore on OSX/Apple
IF(APPLE)
    SET(OKULAR_IOKIT "-framework IOKit" CACHE STRING "Apple IOKit framework")
ENDIF(APPLE)

target_link_libraries(okularcore  ${OKULAR_IOKIT} ${KDE4_KIO_LIBS} ${KDE4_PHONON_LIBRARY} ${MATH_LIB} ${KDE4_THREADWEAVER_LIBRARY} )

set_target_properties(okularcore PROPERTIES VERSION 1.0.0 SOVERSION 1 )

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
   ui/annotationmodel.cpp
   ui/annotationpopup.cpp
   ui/annotationpropertiesdialog.cpp
   ui/annotationproxymodels.cpp
   ui/annotationtools.cpp
   ui/annotationwidgets.cpp
   ui/bookmarklist.cpp
   ui/fileprinterpreview.cpp
   ui/findbar.cpp
   ui/formwidgets.cpp
   ui/guiutils.cpp
   ui/ktreeviewsearchline.cpp
   ui/minibar.cpp
   ui/pageitemdelegate.cpp
   ui/pagepainter.cpp
   ui/pagesizelabel.cpp
   ui/pageviewannotator.cpp
   ui/pageview.cpp
   ui/pageviewutils.cpp
   ui/presentationsearchbar.cpp
   ui/presentationwidget.cpp
   ui/propertiesdialog.cpp
   ui/searchlineedit.cpp
   ui/searchwidget.cpp
   ui/sidebar.cpp
   ui/side_reviews.cpp
   ui/thumbnaillist.cpp
   ui/toc.cpp
   ui/tocmodel.cpp
   ui/toolaction.cpp
)

kde4_add_ui_files(okularpart_SRCS
   conf/dlgaccessibilitybase.ui
   conf/dlggeneralbase.ui
   conf/dlgidentitybase.ui
   conf/dlgperformancebase.ui
   conf/dlgpresentationbase.ui
)


kde4_add_plugin(okularpart ${okularpart_SRCS})

target_link_libraries(okularpart okularcore ${KDE4_KPARTS_LIBS} ${KDE4_KUTILS_LIBS} ${KDE4_KNEWSTUFF2_LIBS} ${MATH_LIB} ${BLITZ_LIBRARIES})

install(TARGETS okularpart DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES okular_part.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES part.rc DESTINATION ${DATA_INSTALL_DIR}/okular )

kde4_install_icons( ${ICON_INSTALL_DIR}   )

