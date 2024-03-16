
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_BINARY_DIR}/../
)

# okular

set(okular_SRCS
   main.cpp
   okular_main.cpp
   shell.cpp
   shellutils.cpp
   recentitemsmodel.cpp
   welcomescreen.cpp

   shell.qrc
)

ki18n_wrap_ui(okular_SRCS
    welcomescreen.ui)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../icons/*-apps-okular.png")
ecm_add_app_icon(okular_SRCS ICONS ${ICONS_SRCS})

add_executable(okular ${okular_SRCS})

target_link_libraries(okular KF6::I18n KF6::Parts KF6::WindowSystem KF6::Crash KF6::IconThemes KF6::KIOGui)
if (TARGET Qt6::DBus)
    target_link_libraries(okular Qt6::DBus)
endif()
if(TARGET Plasma::Activities)
    target_compile_definitions(okular PUBLIC -DWITH_KACTIVITIES=1)
    target_link_libraries(okular Plasma::Activities)
endif()

if (HAVE_X11)
    target_link_libraries(okular Qt6::GuiPrivate)
endif()

install(TARGETS okular ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

if(APPLE)
    set_target_properties(okular PROPERTIES
        MACOSX_BUNDLE_DISPLAY_NAME "Okular"
        MACOSX_BUNDLE_BUNDLE_NAME "Okular"
        MACOSX_BUNDLE_LONG_VERSION_STRING "Okular ${RELEASE_SERVICE_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${RELEASE_SERVICE_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.okular"
        MACOSX_BUNDLE_COPYRIGHT "2002-2024 The Okular Developers")
endif()

# okular shell data files

install( PROGRAMS org.kde.okular.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES org.kde.okular.appdata.xml DESTINATION  ${KDE_INSTALL_METAINFODIR} )
