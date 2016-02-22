cmake_minimum_required(VERSION 2.8.9)
project(okular)

cmake_minimum_required(VERSION 2.8.12)

set(QT_REQUIRED_VERSION "5.2.0")
set(KF5_REQUIRED_VERSION "5.16.0")

find_package(ECM 1.7.0 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMOptionalAddSubdirectory)
include(GenerateExportHeader)
include(FeatureSummary)
include(ECMAddAppIcon)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(ECMAddTests)
include(ECMAddAppIcon) 
include(ECMPackageConfigHelpers)

ecm_setup_version(1.0.0 VARIABLE_PREFIX OKULAR
                        VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/core/version.h"
                        PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/Okular5ConfigVersion.cmake")

find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED COMPONENTS Core DBus Test Widgets PrintSupport Svg Qml Quick)
find_package(Qt5 ${QT_REQUIRED_VERSION} OPTIONAL_COMPONENTS TextToSpeech)
if (NOT Qt5TextToSpeech_FOUND)
    message(STATUS "Qt5TextToSpeech not found, speech features will be disabled")
else()
    add_definitions(-DHAVE_SPEECH)
endif()

find_package(KF5 ${KF5_REQUIRED_VERSION} REQUIRED COMPONENTS
    Activities
    Archive
    Bookmarks
    Completion
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

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
add_definitions(-DTRANSLATION_DOMAIN="okular")

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${PHONON_INCLUDES} core/synctex ${ZLIB_INCLUDE_DIR} ${CMAKE_BINARY_DIR}/core)

add_subdirectory( mobile )
option(BUILD_COVERAGE "Build the project with gcov support" OFF)

if(BUILD_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgcov")
endif()

add_subdirectory( ui )
add_subdirectory( shell )
add_subdirectory( generators )
add_subdirectory( autotests )
add_subdirectory( conf/autotests )

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
           core/fileprinter.h
           core/observer.h
           ${CMAKE_CURRENT_BINARY_DIR}/core/version.h
           ${CMAKE_CURRENT_BINARY_DIR}/core/okularcore_export.h
           ${CMAKE_CURRENT_BINARY_DIR}/settings_core.h
         DESTINATION ${KDE_INSTALL_INCLUDEDIR}/okular/core COMPONENT Devel)

install( FILES
           interfaces/configinterface.h
           interfaces/guiinterface.h
           interfaces/printinterface.h
           interfaces/saveinterface.h
           interfaces/viewerinterface.h
         DESTINATION ${KDE_INSTALL_INCLUDEDIR}/okular/interfaces COMPONENT Devel)

ki18n_wrap_ui(okularcore_SRCS
   core/chooseenginewidget.ui
)

kconfig_add_kcfg_files(okularcore_SRCS conf/settings_core.kcfgc )

add_library(okularcore SHARED ${okularcore_SRCS})
generate_export_header(okularcore BASE_NAME okularcore EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/core/okularcore_export.h")

# Special handling for linking okularcore on OSX/Apple
IF(APPLE)
    SET(OKULAR_IOKIT "-framework IOKit" CACHE STRING "Apple IOKit framework")
ENDIF(APPLE)

target_link_libraries(okularcore
PRIVATE
    ${OKULAR_IOKIT}
    KF5::Archive
    KF5::JS
    KF5::JSApi
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::I18n
    KF5::ThreadWeaver
    KF5::Wallet
    KF5::Bookmarks
    Phonon::phonon4qt5
    ${MATH_LIB}
    ${ZLIB_LIBRARIES}
PUBLIC  # these are included from the installed headers
    KF5::CoreAddons
    KF5::XmlGui
    KF5::ConfigGui
    Qt5::PrintSupport
    Qt5::Widgets
)

set_target_properties(okularcore PROPERTIES VERSION 7.0.0 SOVERSION 7 OUTPUT_NAME Okular5Core EXPORT_NAME Core)

install(TARGETS okularcore EXPORT Okular5Targets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES conf/okular.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES conf/okular_core.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES core/okularGenerator.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})

# okularpart

set(okularpart_conf_SRCS
   conf/preferencesdialog.cpp
   conf/dlgaccessibility.cpp
   conf/dlgdebug.cpp
   conf/dlgeditor.cpp
   conf/dlggeneral.cpp
   conf/dlgannotations.cpp
   conf/dlgperformance.cpp
   conf/dlgpresentation.cpp
   conf/editannottooldialog.cpp
   conf/editdrawingtooldialog.cpp
   conf/widgetannottools.cpp
   conf/widgetconfigurationtoolsbase.cpp
   conf/widgetdrawingtools.cpp
)

set(okularpart_SRCS
   ${okularpart_conf_SRCS}
   part.cpp
   extensions.cpp
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
   ui/drawingtoolactions.cpp
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
   ui/videowidget.cpp
   ui/layers.cpp
)

if (Qt5TextToSpeech_FOUND)
   set(okularpart_SRCS ${okularpart_SRCS}
      ui/tts.cpp)
endif()

ki18n_wrap_ui(okularpart_SRCS
   conf/dlgaccessibilitybase.ui
   conf/dlgeditorbase.ui
   conf/dlggeneralbase.ui
   conf/dlgannotationsbase.ui
   conf/dlgperformancebase.ui
   conf/dlgpresentationbase.ui
)

kconfig_add_kcfg_files(okularpart_SRCS conf/settings.kcfgc )

add_library(okularpart SHARED ${okularpart_SRCS})
generate_export_header(okularpart BASE_NAME okularpart)

target_link_libraries(okularpart okularcore Qt5::Svg KF5::Parts ${MATH_LIB} Phonon::phonon4qt5 KF5::Solid KF5::KDELibs4Support KF5::Wallet KF5::Archive)

if (Qt5TextToSpeech_FOUND)
   target_link_libraries(okularpart Qt5::TextToSpeech)
endif()
install(TARGETS okularpart DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### install files ###############

install(FILES okular.upd DESTINATION ${KDE_INSTALL_DATADIR}/kconf_update)

install( FILES okular_part.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( FILES part.rc part-viewermode.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/okular )

install( FILES okular.categories DESTINATION ${KDE_INSTALL_CONFDIR} )

########### cmake files #################

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/Okular5")
ecm_configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Okular5Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Okular5Config.cmake"
  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
  PATH_VARS  INCLUDE_INSTALL_DIR CMAKE_INSTALL_PREFIX
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/Okular5Config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/Okular5ConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT Okular5Targets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE Okular5Targets.cmake NAMESPACE Okular::)

########### summary #################

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

