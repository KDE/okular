
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# checking whether we should enable activities support
find_package(KF5Activities)
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

kde4_add_app_icon(okular_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../ui/data/icons/hi*-apps-okular.png")

add_executable(okular ${okular_SRCS})

target_link_libraries(okular KF5::Parts KF5::WindowSystem KF5::KDELibs4Support)

if(KActivities_FOUND)
 add_definitions(-DKActivities_FOUND)
 include_directories(${KACTIVITIES_INCLUDE_DIRS})
 target_link_libraries(okular KF5::KActivities)
endif(KActivities_FOUND)

install(TARGETS okular ${INSTALL_TARGETS_DEFAULT_ARGS})


# okular shell data files

install( PROGRAMS org.kde.okular.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES shell.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/okular )
