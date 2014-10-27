project(okular)



cmake_minimum_required(VERSION 2.8.12)

set(QT_MIN_VERSION "5.2.0")

find_package(ECM 1.2.0 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMOptionalAddSubdirectory)
include(GenerateExportHeader)
include(FeatureSummary)
include(KDEFrameworkCompilerSettings)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(ECMAddTests)


find_package(Qt5 CONFIG REQUIRED COMPONENTS Core DBus Test Widgets PrintSupport Svg)
find_package(KF5 REQUIRED COMPONENTS
    Activities
    Archive
    Bookmarks
    Config
    ConfigWidgets
    CoreAddons
    DBusAddons
    JS
    KDELibs4Support
    KIO
    Parts
    ThreadWeaver
    Wallet
    KHtml
)
find_package(Phonon4Qt5 CONFIG REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules)

find_package(ZLIB REQUIRED)

# There was no Qt 5 version at the time of porting
#find_package(QImageBlitz)
#set_package_properties("QImageBlitz" PROPERTIES
#        DESCRIPTION "An image effects library"
#        URL "http://sourceforge.net/projects/qimageblitz"
#        TYPE RECOMMENDED
#        PURPOSE "Required to build Okular.")

find_package(KF5Screen)
set_package_properties("LibKScreen" PROPERTIES
        DESCRIPTION "KDE screen management library"
        URL "https://projects.kde.org/projects/kdereview/libkscreen"
        TYPE RECOMMENDED
        PURPOSE "DPI detection support")

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
add_definitions(-DTRANSLATION_DOMAIN="okular")

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${PHONON_INCLUDES} core/synctex ${ZLIB_INCLUDE_DIR})

message("TODO: when core compiles add subdirs")
#add_subdirectory( active )
add_subdirectory( ui )
add_subdirectory( shell )
add_subdirectory( generators )
add_subdirectory( autotests )

add_subdirectory(doc)

include(OkularConfigureChecks.cmake)

if(NOT WIN32)
 set(MATH_LIB m)
else(NOT WIN32)
 set(MATH_LIB)
endif(NOT WIN32)

# okularcore

set(okularcore_SRCS
   core/action.cpp
   core/annotations.cpp
   core/area.cpp
   core/audioplayer.cpp
   core/bookmarkmanager.cpp
   core/chooseenginedialog.cpp
   core/document.cpp
   core/documentcommands.cpp
   core/fontinfo.cpp
   core/form.cpp
   core/generator.cpp
   core/generator_p.cpp
   core/misc.cpp
   core/movie.cpp
   core/observer.cpp
   core/debug.cpp
   core/page.cpp
   core/pagecontroller.cpp
   core/pagesize.cpp
   core/pagetransition.cpp
   core/rotationjob.cpp
   core/scripter.cpp
   core/sound.cpp
   core/sourcereference.cpp
   core/textdocumentgenerator.cpp
   core/textdocumentsettings.cpp
   core/textpage.cpp
   core/tilesmanager.cpp
   core/utils.cpp
   core/view.cpp
   core/fileprinter.cpp
   core/script/executor_kjs.cpp
   core/script/kjs_app.cpp
   core/script/kjs_console.cpp
   core/script/kjs_data.cpp
   core/script/kjs_document.cpp
   core/script/kjs_fullscreen.cpp
   core/script/kjs_field.cpp
   core/script/kjs_spell.cpp
   core/script/kjs_util.cpp
   core/synctex/synctex_parser.c
   core/synctex/synctex_parser_utils.c
)

ki18n_wrap_ui(okularcore_SRCS
   conf/textdocumentsettings.ui
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
           core/okular_export.h
           core/page.h
           core/pagesize.h
           core/pagetransition.h
           core/sound.h
           core/sourcereference.h
           core/textdocumentgenerator.h
           core/textdocumentsettings.h
           core/textpage.h
           core/tile.h
           core/utils.h
           core/version.h
           core/fileprinter.h
           core/observer.h
           ${CMAKE_CURRENT_BINARY_DIR}/settings_core.h
         DESTINATION ${INCLUDE_INSTALL_DIR}/okular/core COMPONENT Devel)

install( FILES
           interfaces/configinterface.h
           interfaces/guiinterface.h
           interfaces/printinterface.h
           interfaces/saveinterface.h
           interfaces/viewerinterface.h
         DESTINATION ${INCLUDE_INSTALL_DIR}/okular/interfaces COMPONENT Devel)

ki18n_wrap_ui(okularcore_SRCS
   core/chooseenginewidget.ui
)

kconfig_add_kcfg_files(okularcore_SRCS conf/settings_core.kcfgc )

add_library(okularcore SHARED ${okularcore_SRCS})
generate_export_header(okularcore BASE_NAME okularcore)

# Special handling for linking okularcore on OSX/Apple
IF(APPLE)
    SET(OKULAR_IOKIT "-framework IOKit" CACHE STRING "Apple IOKit framework")
ENDIF(APPLE)

target_link_libraries(okularcore
    ${OKULAR_IOKIT}
    KF5::Archive
    KF5::JS
    KF5::JSApi
    KF5::KDELibs4Support
    KF5::KIOCore
    KF5::ThreadWeaver
    KF5::Wallet
    Phonon::phonon4qt5
    ${MATH_LIB}
    ${ZLIB_LIBRARIES}
)

if(LibKScreen_FOUND)
#    target_link_libraries(okularcore KF5::LibKScreen)
endif(LibKScreen_FOUND)

set_target_properties(okularcore PROPERTIES VERSION 5.0.0 SOVERSION 5 )

install(TARGETS okularcore ${INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES conf/okular.kcfg DESTINATION ${KCFG_INSTALL_DIR})
install(FILES conf/okular_core.kcfg DESTINATION ${KCFG_INSTALL_DIR})
install(FILES core/okularGenerator.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})

# okularpart

set(okularpart_SRCS
   part.cpp
   extensions.cpp
   conf/preferencesdialog.cpp
   conf/dlgaccessibility.cpp
   conf/dlgdebug.cpp
   conf/dlgeditor.cpp
   conf/dlggeneral.cpp
   conf/dlgannotations.cpp
   conf/dlgperformance.cpp
   conf/dlgpresentation.cpp
   conf/widgetannottools.cpp
   ui/embeddedfilesdialog.cpp
   ui/annotwindow.cpp
   ui/annotationmodel.cpp
   ui/annotationpopup.cpp
   ui/annotationpropertiesdialog.cpp
   ui/annotationproxymodels.cpp
   ui/annotationtools.cpp
   ui/annotationwidgets.cpp
   ui/bookmarklist.cpp
   ui/debug_ui.cpp
   ui/fileprinterpreview.cpp
   ui/findbar.cpp
   ui/formwidgets.cpp
   ui/guiutils.cpp
   ui/ktreeviewsearchline.cpp
   ui/latexrenderer.cpp
   ui/minibar.cpp
   ui/pageitemdelegate.cpp
   ui/pagepainter.cpp
   ui/pagesizelabel.cpp
   ui/pageviewannotator.cpp
   ui/pageview.cpp
   ui/magnifierview.cpp
   ui/pageviewutils.cpp
   ui/presentationsearchbar.cpp
   ui/presentationwidget.cpp
   ui/propertiesdialog.cpp
   ui/searchlineedit.cpp
   ui/searchwidget.cpp
   ui/sidebar.cpp
   ui/side_reviews.cpp
   ui/snapshottaker.cpp
   ui/thumbnaillist.cpp
   ui/toc.cpp
   ui/tocmodel.cpp
   ui/toolaction.cpp
#   ui/tts.cpp
   ui/videowidget.cpp
)

ki18n_wrap_ui(okularpart_SRCS
   conf/dlgaccessibilitybase.ui
   conf/dlgeditorbase.ui
   conf/dlggeneralbase.ui
   conf/dlgannotationsbase.ui
   conf/dlgperformancebase.ui
   conf/dlgpresentationbase.ui
)

kconfig_add_kcfg_files(okularpart_SRCS conf/settings.kcfgc )

message("KF5: Enable ui/tts.cpp again")
#qt5_add_dbus_interfaces(okularpart_SRCS ${KDE4_DBUS_INTERFACES_DIR}/org.kde.KSpeech.xml)

add_library(okularpart MODULE ${okularpart_SRCS})
generate_export_header(okularpart BASE_NAME okularpart)

target_link_libraries(okularpart okularcore Qt5::Svg KF5::Parts ${MATH_LIB} Phonon::phonon4qt5 KF5::Solid)

install(TARGETS okularpart DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install(FILES okular.upd DESTINATION ${DATA_INSTALL_DIR}/kconf_update)

install( FILES okular_part.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES part.rc part-viewermode.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/okular )

########### cmake files #################

include(WriteBasicConfigVersionFile)
write_basic_config_version_file(${CMAKE_CURRENT_BINARY_DIR}/OkularConfigVersion.cmake VERSION 1.0.0 COMPATIBILITY AnyNewerVersion)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/OkularConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/OkularConfigVersion.cmake
        DESTINATION ${LIB_INSTALL_DIR}/cmake/Okular )

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

