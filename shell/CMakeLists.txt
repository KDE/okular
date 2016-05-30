
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_BINARY_DIR}/../
)

set_package_properties("KActivities" PROPERTIES
        DESCRIPTION  "Activities interface library"
        URL "https://projects.kde.org/projects/kde/kdelibs/kactivities"
        TYPE RECOMMENDED
        PURPOSE "Required for Activities integration.")

# okular

set(okular_SRCS
   main.cpp
   okular_main.cpp
   shell.cpp
   shellutils.cpp
)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../ui/data/icons/*-apps-okular.png")
ecm_add_app_icon(okular_SRCS ICONS ${ICONS_SRCS})

add_executable(okular ${okular_SRCS})

target_link_libraries(okular KF5::Parts KF5::WindowSystem KF5::Activities)

install(TARGETS okular ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


# okular shell data files

install( PROGRAMS org.kde.okular.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES shell.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/okular )
