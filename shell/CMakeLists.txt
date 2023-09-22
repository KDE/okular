
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

target_link_libraries(okular KF6::I18n KF6::Parts KF6::WindowSystem KF6::Crash KF6::IconThemes)
if (TARGET Qt6::DBus)
    target_link_libraries(okular Qt6::DBus)
endif()
if(TARGET KF6::Activities)
    target_compile_definitions(okular PUBLIC -DWITH_KACTIVITIES=1)

	target_link_libraries(okular KF6::Activities)
endif()

if (HAVE_X11)
    target_link_libraries(okular Qt6::GuiPrivate)
endif()

install(TARGETS okular ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


# okular shell data files

install( PROGRAMS org.kde.okular.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES org.kde.okular.appdata.xml DESTINATION  ${KDE_INSTALL_METAINFODIR} )
