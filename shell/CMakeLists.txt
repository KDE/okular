
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# checking whether we should enable activities support
find_package(KActivities "6.1.0")
set_package_properties("KActivities" PROPERTIES
        DESCRIPTION  "Activities interface library"
        URL "https://projects.kde.org/projects/kde/kdelibs/kactivities"
        TYPE RECOMMENDED
        PURPOSE "Required for Activities integration.")

if(KActivities_FOUND)
 add_definitions(-DKActivities_FOUND)
 include_directories(${KACTIVITIES_INCLUDE_DIRS})
endif(KActivities_FOUND)

# okular

set(okular_SRCS
   main.cpp
   shell.cpp
   shellutils.cpp
)

kde4_add_app_icon(okular_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../ui/data/icons/hi*-apps-okular.png")

kde4_add_executable(okular ${okular_SRCS})

target_link_libraries(okular KF5::Parts KF5::KActivities)

install(TARGETS okular ${INSTALL_TARGETS_DEFAULT_ARGS})


# okular shell data files

install( PROGRAMS okular.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES shell.rc DESTINATION ${DATA_INSTALL_DIR}/okular )

