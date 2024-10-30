cmake_minimum_required(VERSION 3.22)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "24")
set (RELEASE_SERVICE_VERSION_MINOR "08")
set (RELEASE_SERVICE_VERSION_MICRO "3")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(okular VERSION ${RELEASE_SERVICE_VERSION})

set(QT_REQUIRED_VERSION "6.6.0") # Remember to update the QT_DEPRECATED_WARNINGS_SINCE below
set(KF_REQUIRED_VERSION "5.240.0") # Remember to update the KF_DEPRECATED_WARNINGS_SINCE below

set(OKULAR_UI "" CACHE STRING "Which Okular user interface to build. Possible values: desktop, mobile, both. Default: desktop (except on Android, where it is 'mobile')")

set(FORCE_NOT_REQUIRED_DEPENDENCIES "" CACHE STRING "List (semicolon-separated) of dependencies that will be downgraded from REQUIRED to RECOMMENDED")

find_package(ECM ${KF_REQUIRED_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMOptionalAddSubdirectory)
include(GenerateExportHeader)
include(FeatureSummary)
include(ECMAddAppIcon)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(ECMAddTests)
include(ECMAddAppIcon)
include(CMakePackageConfigHelpers)
include(ECMSetupQtPluginMacroNames)
include(ECMAddAndroidApk)

if (ANDROID AND (FORCE_NOT_REQUIRED_DEPENDENCIES STREQUAL ""))
    set(FORCE_NOT_REQUIRED_DEPENDENCIES "KF6Wallet;KF6DocTools;Qt6Qml;TIFF;LibSpectre;KExiv2Qt6;CHM;LibZip;DjVuLibre;EPub;Discount;")
endif()

function(set_okular_optional_package_properties _name _props)
    if(NOT "${_props}" STREQUAL "PROPERTIES")
        message(FATAL_ERROR "PROPERTIES keyword is missing in set_okular_optional_package_properties() call.")
    endif()

    set(options) # none
    set(oneValueArgs DESCRIPTION URL PURPOSE)
    set(multiValueArgs) # none

    CMAKE_PARSE_ARGUMENTS(_SPP "${options}" "${oneValueArgs}" "${multiValueArgs}"  ${ARGN})

    if(_SPP_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to set_okular_optional_package_properties(): \"${_SPP_UNPARSED_ARGUMENTS}\"")
    endif()

    set(DEPENDENCY_TYPE "REQUIRED")
    if (${_name} IN_LIST FORCE_NOT_REQUIRED_DEPENDENCIES)
        set(DEPENDENCY_TYPE "RECOMMENDED")
    endif()
    set_package_properties(${_name} PROPERTIES
        TYPE ${DEPENDENCY_TYPE}
        DESCRIPTION ${_SPP_DESCRIPTION}
        URL ${_SPP_URL}
        PURPOSE "${_SPP_PURPOSE} You can make the dependency optional adding ${_name} to the FORCE_NOT_REQUIRED_DEPENDENCIES cmake option"
    )
endfunction()

if(OKULAR_UI STREQUAL "")
    if(ANDROID)
        set(OKULAR_UI "mobile")
    else()
        set(OKULAR_UI "desktop")
    endif()
endif()

if(OKULAR_UI STREQUAL "desktop" OR OKULAR_UI STREQUAL "both")
    set(BUILD_DESKTOP ON)
else()
    set(BUILD_DESKTOP OFF)
endif()
if(OKULAR_UI STREQUAL "mobile" OR OKULAR_UI STREQUAL "both")
    set(BUILD_MOBILE ON)
else()
    set(BUILD_MOBILE OFF)
endif()

# append the plugins from the install tree folder to the QT_PLUGIN_PATH
if(UNIX)
    set(ENV{QT_PLUGIN_PATH} "$ENV{QT_PLUGIN_PATH}:${KDE_INSTALL_FULL_QTPLUGINDIR}")
elseif(WIN32)
    set(ENV{QT_PLUGIN_PATH} "$ENV{QT_PLUGIN_PATH};${KDE_INSTALL_FULL_QTPLUGINDIR}")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

ecm_setup_version(${PROJECT_VERSION}
                  VARIABLE_PREFIX OKULAR
                  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/core/version.h"
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/Okular6ConfigVersion.cmake")

find_package(Qt6 ${QT_REQUIRED_VERSION} CONFIG REQUIRED COMPONENTS Core Test Widgets PrintSupport Svg Xml)

ecm_setup_qtplugin_macro_names(
    JSON_ARG2
        "OKULAR_EXPORT_PLUGIN"
    CONFIG_CODE_VARIABLE
        PACKAGE_SETUP_AUTOMOC_VARIABLES
)

find_package(KF6 ${KF_REQUIRED_VERSION} REQUIRED COMPONENTS
    Archive
    Bookmarks
    Completion
    Config
    ConfigWidgets
    CoreAddons
    I18n
    KIO
    ThreadWeaver
    WindowSystem
    XmlGui
)

if (NOT WIN32)
    find_package(KF6Wallet ${KF_REQUIRED_VERSION})
    set_okular_optional_package_properties(KF6Wallet PROPERTIES
    PURPOSE "Required for document storing passwords in secure wallets.")
endif()
if(KF6Wallet_FOUND)
    set(HAVE_KWALLET 1)
else()
    set(HAVE_KWALLET 0)
endif()

find_package(KF6DocTools ${KF_REQUIRED_VERSION})
set_okular_optional_package_properties(KF6DocTools PROPERTIES
    PURPOSE "Required for compiling and installing the user documentation.")

find_package(Qt6Qml)
set_okular_optional_package_properties(Qt6Qml PROPERTIES
    PURPOSE "Required for supporting JavaScript in PDF documents")
if(TARGET Qt6::Qml)
    set(HAVE_JS 1)
else()
    set(HAVE_JS 0)
endif()

if (BUILD_DESKTOP)
    # shall we use DBus?
    # enabled per default on Linux & BSD systems
    set(USE_DBUS_DEFAULT OFF)
    if(UNIX AND NOT APPLE AND NOT ANDROID AND NOT HAIKU)
        set(USE_DBUS_DEFAULT ON)
    endif()
    option(USE_DBUS "Build components using DBus (used for interprocess communication, external open in same instance and various presentation related bits)" ${USE_DBUS_DEFAULT})
    if(USE_DBUS)
        find_package(Qt6DBus REQUIRED)
        set(HAVE_DBUS 1)
    endif()

    find_package(KF6 ${KF_REQUIRED_VERSION} REQUIRED COMPONENTS Parts ColorScheme Crash IconThemes ItemViews TextWidgets WidgetsAddons)

    find_package(KF6Purpose)
    set_okular_optional_package_properties(KF6Purpose PROPERTIES
        DESCRIPTION "A framework for services and actions integration"
        PURPOSE "Required for enabling the share menu.")
    if (KF6Purpose_FOUND)
        set(HAVE_PURPOSE 1)
    else()
        set(HAVE_PURPOSE 0)
    endif()

    find_package(Qt6TextToSpeech ${QT_REQUIRED_VERSION} CONFIG)
    set_okular_optional_package_properties(Qt6TextToSpeech PROPERTIES
        PURPOSE "Enables speech features.")
    if (Qt6TextToSpeech_FOUND)
        set(HAVE_SPEECH 1)
    else()
        set(HAVE_SPEECH 0)
    endif()
endif()

if (BUILD_MOBILE)
    find_package(Qt6 ${QT_REQUIRED_VERSION} CONFIG REQUIRED COMPONENTS Qml Quick)
    find_package(KF6Kirigami2)
    set_package_properties(KF6Kirigami2 PROPERTIES
        DESCRIPTION "A QtQuick based components set"
        PURPOSE "Required at runtime by the mobile app"
        TYPE RUNTIME
    )
    find_package(KF6KirigamiAddons)
    set_package_properties(KF6KirigamiAddons PROPERTIES
        DESCRIPTION "A set of widgets for Kirigami along with  code to support them"
        PURPOSE "Required at runtime by the mobile app"
        TYPE RUNTIME
    )
endif()

if(NOT WIN32 AND NOT ANDROID AND NOT APPLE)
    find_package(Qt6Gui REQUIRED COMPONENTS Private)
    find_package(PlasmaActivities REQUIRED)
    set_package_properties("PlasmaActivities" PROPERTIES
        DESCRIPTION  "Activities interface library"
        URL "https://invent.kde.org/plasma/plasma-activities"
        PURPOSE "Required for Activities integration."
    )
    set(HAVE_X11 TRUE)
else()
    set(HAVE_X11 FALSE)
endif()
find_package(Phonon4Qt6 CONFIG)
set_okular_optional_package_properties(Phonon4Qt6 PROPERTIES
    PURPOSE "Required for documents with audio or video elements")
if (Phonon4Qt6_FOUND)
    set(HAVE_PHONON 1)
else()
    set(HAVE_PHONON 0)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules)

find_package(ZLIB REQUIRED)

find_package(Poppler "22.02.0" COMPONENTS Qt6)
set_okular_optional_package_properties(Poppler PROPERTIES
        PURPOSE "Support for PDF files in okular.")

find_package(Freetype)
set_okular_optional_package_properties(Freetype PROPERTIES
        DESCRIPTION  "A font rendering engine"
        URL "https://www.freetype.org"
        PURPOSE "Provides freetype font support in the okular DVI generator.")

find_package(TIFF)
set_okular_optional_package_properties(TIFF PROPERTIES
        DESCRIPTION  "A library for reading and writing TIFF formatted files,"
        URL "http://www.libtiff.org"
        PURPOSE "Support for TIFF files in okular.")

set(LIBSPECTRE_MINIMUM_VERSION "0.2")
find_package(LibSpectre "${LIBSPECTRE_MINIMUM_VERSION}")
set_okular_optional_package_properties(LibSpectre PROPERTIES
        DESCRIPTION  "A PostScript rendering library"
        URL "https://libspectre.freedesktop.org"
        PURPOSE "Support for PS files in okular.")

find_package(KExiv2Qt6 CONFIG)
set_okular_optional_package_properties(KExiv2Qt6 PROPERTIES
        DESCRIPTION  "Wrapper around Exiv2 library"
        URL "https://commits.kde.org/libkexiv2"
        PURPOSE "Support for exif rotation in image files.")

if (FALSE ) # Investigate porting at some point
    find_package(CHM)
    set_okular_optional_package_properties(CHM PROPERTIES
            DESCRIPTION  "A library for dealing with Microsoft ITSS/CHM format files"
            URL "http://www.jedrea.com/chmlib"
            PURPOSE "Support CHM files in okular.")

endif()

find_package(LibZip)
set_okular_optional_package_properties(LibZip PROPERTIES
        DESCRIPTION  "A library for reading, creating, and modifying zip archives"
        URL "https://libzip.org/"
        PURPOSE "Support CHM files in okular.")

find_package(DjVuLibre "3.5.28")
set_okular_optional_package_properties(DjVuLibre PROPERTIES
        DESCRIPTION  "A library for dealing with DjVu formatted files"
        URL "https://djvulibre.djvuzone.org"
        PURPOSE "Support for DjVu files in okular.")

find_package(EPub)
set_okular_optional_package_properties(EPub PROPERTIES
        DESCRIPTION  "A library for reading EPub documents"
        URL "http://sourceforge.net/projects/ebook-tools"
        PURPOSE "Support for EPub documents in Okular.")

find_package(QMobipocket6 "2" CONFIG)
set_okular_optional_package_properties(QMobiPocket6 PROPERTIES
        DESCRIPTION  "A library for reading Mobipocket documents"
        URL "https://commits.kde.org/kdegraphics-mobipocket"
        PURPOSE "Support for Mobipocket documents in Okular.")

find_package(Discount)
set_okular_optional_package_properties(Discount PROPERTIES
        DESCRIPTION "A library that gives you formatting functions suitable for marking down entire documents or lines of text"
        URL "https://www.pell.portland.or.us/~orc/Code/discount/"
        PURPOSE "Support for Markdown documents in Okular.")

add_definitions(-DTRANSLATION_DOMAIN="okular")
add_definitions(-DQT_DEPRECATED_WARNINGS_SINCE=0x060600)
add_definitions(-DKF_DEPRECATED_WARNINGS_SINCE=0x05F000)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${PHONON_INCLUDES} core/synctex ${CMAKE_BINARY_DIR}/core)

option(BUILD_COVERAGE "Build the project with gcov support" OFF)

if(BUILD_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgcov")
endif()

add_subdirectory( icons ) # an own directory for multi-size icons retrieved by KIconThemeLoader
add_subdirectory( part )
if(BUILD_DESKTOP)
    add_subdirectory( shell )
endif()
add_subdirectory( generators )

if(BUILD_MOBILE)
    add_subdirectory( mobile )
endif()

if(BUILD_TESTING)
   add_subdirectory( autotests )
endif()

if(KF6DocTools_FOUND)
    add_subdirectory(doc)
endif()

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
   core/printoptionswidget.cpp
   core/signatureutils.cpp
   core/script/event.cpp
   core/synctex/synctex_parser.c
   core/synctex/synctex_parser_utils.c
)
qt_add_resources(okularcore_SRCS
   core/script/builtin.qrc
)

ki18n_wrap_ui(okularcore_SRCS
   core/textdocumentsettings.ui
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
           core/signatureutils.h
           core/sound.h
           core/sourcereference.h
           core/textdocumentgenerator.h
           core/textdocumentsettings.h
           core/textpage.h
           core/tile.h
           core/utils.h
           core/fileprinter.h
           core/printoptionswidget.h
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

kconfig_add_kcfg_files(okularcore_SRCS GENERATE_MOC conf/settings_core.kcfgc)

add_library(okularcore SHARED ${okularcore_SRCS})
generate_export_header(okularcore BASE_NAME okularcore EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/core/okularcore_export.h")
target_include_directories(okularcore PUBLIC "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}>")

if (ANDROID)
    set(fileName ${CMAKE_BINARY_DIR}/Okular5Core-android-dependencies.xml)
    file(WRITE "${fileName}" "<rules><dependencies><lib name='Okular5Core'><depends>\n"
                                "<bundled file='${KDE_INSTALL_PLUGINDIR}/okular/generators' />\n"
                             "</depends></lib></dependencies></rules>\n")
    install(FILES ${fileName} DESTINATION ${KDE_INSTALL_LIBDIR})
endif()


# Special handling for linking okularcore on OSX/Apple
IF(APPLE)
    SET(OKULAR_IOKIT "-framework IOKit" CACHE STRING "Apple IOKit framework")
ENDIF(APPLE)

# Extra library needed by imported synctex code on Windows
if(WIN32)
    set(SHLWAPI shlwapi)
endif(WIN32)

target_link_libraries(okularcore
PRIVATE
    ${OKULAR_IOKIT}
    ${SHLWAPI}
    KF6::Archive
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::I18n
    KF6::ThreadWeaver
    KF6::BookmarksWidgets
    KF6::Bookmarks
    Qt6::Svg
    ${MATH_LIB}
    ZLIB::ZLIB
PUBLIC  # these are included from the installed headers
    KF6::CoreAddons
    KF6::XmlGui
    KF6::ConfigGui
    Qt6::PrintSupport
    Qt6::Widgets
)
if (Phonon4Qt6_FOUND)
    target_link_libraries(okularcore PRIVATE Phonon::phonon4qt6)
endif()


if (KF6Wallet_FOUND)
    target_link_libraries(okularcore PRIVATE KF6::Wallet)
endif()
if (TARGET Qt6::Qml)
    target_sources(okularcore PRIVATE
            core/script/executor_js.cpp
            core/script/js_app.cpp
            core/script/js_console.cpp
            core/script/js_data.cpp
            core/script/js_display.cpp
            core/script/js_document.cpp
            core/script/js_field.cpp
            core/script/js_fullscreen.cpp
            core/script/js_field.cpp
            core/script/js_spell.cpp
            core/script/js_util.cpp
            core/script/js_event.cpp
            core/script/js_ocg.cpp
            core/script/js_global.cpp
    )
    target_link_libraries(okularcore PRIVATE Qt6::Qml)
endif()

set_target_properties(okularcore PROPERTIES VERSION 2.0.0 SOVERSION 2 OUTPUT_NAME Okular6Core EXPORT_NAME Core)

install(TARGETS okularcore EXPORT Okular6Targets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES conf/okular.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES conf/okular_core.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})

if(BUILD_DESKTOP)
  # okularpart
  set(okularpart_SRCS
    gui/certificatemodel.cpp
    gui/debug_ui.cpp
    gui/guiutils.cpp
    gui/pagepainter.cpp
    gui/signatureguiutils.cpp
    gui/signaturemodel.cpp
    gui/tocmodel.cpp
    part/preferencesdialog.cpp
    part/dlgaccessibility.cpp
    part/dlgdebug.cpp
    part/dlgeditor.cpp
    part/dlggeneral.cpp
    part/dlgannotations.cpp
    part/dlgperformance.cpp
    part/dlgpresentation.cpp
    part/editannottooldialog.cpp
    part/editdrawingtooldialog.cpp
    part/widgetannottools.cpp
    part/widgetconfigurationtoolsbase.cpp
    part/widgetdrawingtools.cpp
    part/part.cpp
    part/extensions.cpp
    part/embeddedfilesdialog.cpp
    part/actionbar.cpp
    part/annotationactionhandler.cpp
    part/annotwindow.cpp
    part/annotationmodel.cpp
    part/annotationpopup.cpp
    part/annotationpropertiesdialog.cpp
    part/annotationproxymodels.cpp
    part/annotationtools.cpp
    part/annotationwidgets.cpp
    part/bookmarklist.cpp
    part/certificateviewer.cpp
    part/colormodemenu.cpp
    part/cursorwraphelper.cpp
    part/drawingtoolactions.cpp
    part/fileprinterpreview.cpp
    part/findbar.cpp
    part/formwidgets.cpp
    part/ktreeviewsearchline.cpp
    part/latexrenderer.cpp
    part/minibar.cpp
    part/okmenutitle.cpp
    part/pageitemdelegate.cpp
    part/pagesizelabel.cpp
    part/pageviewannotator.cpp
    part/pageviewmouseannotation.cpp
    part/pageview.cpp
    part/magnifierview.cpp
    part/pageviewutils.cpp
    part/presentationsearchbar.cpp
    part/presentationwidget.cpp
    part/propertiesdialog.cpp
    part/revisionviewer.cpp
    part/searchlineedit.cpp
    part/searchwidget.cpp
    part/sidebar.cpp
    part/side_reviews.cpp
    part/snapshottaker.cpp
    part/thumbnaillist.cpp
    part/toc.cpp
    part/toggleactionmenu.cpp
    part/videowidget.cpp
    part/layers.cpp
    part/signaturepartutils.cpp
    part/signaturepropertiesdialog.cpp
    part/signaturepanel.cpp

    part/part.qrc
  )
  ki18n_wrap_ui(okularpart_SRCS part/selectcertificatedialog.ui)

if (Qt6TextToSpeech_FOUND)
   set(okularpart_SRCS ${okularpart_SRCS}
      part/tts.cpp)
endif()

kconfig_add_kcfg_files(okularpart_SRCS GENERATE_MOC conf/settings.kcfgc)

add_library(okularpart SHARED ${okularpart_SRCS})
generate_export_header(okularpart BASE_NAME okularpart)

target_link_libraries(okularpart okularcore
    ${MATH_LIB}
    Qt6::Svg
    KF6::Archive
    KF6::Bookmarks
    KF6::I18n
    KF6::IconThemes
    KF6::ItemViews
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::Parts
    KF6::WindowSystem
    KF6::TextWidgets
)
if (TARGET Qt6::DBus)
    target_link_libraries(okularpart Qt6::DBus)
endif()
if (Phonon4Qt6_FOUND)
   target_link_libraries(okularpart Phonon::phonon4qt6)
endif()

if(KF6Wallet_FOUND)
    target_link_libraries(okularpart KF6::Wallet)
endif()

if (KF6Purpose_FOUND)
    target_link_libraries(okularpart KF6::PurposeWidgets)
endif()

set_target_properties(okularpart PROPERTIES PREFIX "")
set_target_properties(okularpart PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/kf6/parts)

if (Qt6TextToSpeech_FOUND)
   target_link_libraries(okularpart Qt6::TextToSpeech)
endif()

#instead install the part to this path so it can be found
install(TARGETS okularpart DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/parts/ )

endif()# End of BUILD_DESKTOP if


########### install files ###############

install(FILES okular.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(FILES okular.categories  DESTINATION  ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

ki18n_install(po)
if(KF6DocTools_FOUND)
    kdoctools_install(po)
endif()

########### cmake files #################

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/Okular6")
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Okular6Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Okular6Config.cmake"
  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
  PATH_VARS  KDE_INSTALL_INCLUDEDIR CMAKE_INSTALL_PREFIX
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/Okular6Config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/Okular6ConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT Okular6Targets DESTINATION "${CMAKECONFIG_INSTALL_DIR}" FILE Okular6Targets.cmake NAMESPACE Okular::)

install(FILES
   core/stamps.svg
   DESTINATION ${KDE_INSTALL_DATADIR}/okular/pics)

########### summary #################

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

message("-- Building Desktop User Interface: ${BUILD_DESKTOP}")
message("-- Building Mobile Interface: ${BUILD_MOBILE}")
message("")
