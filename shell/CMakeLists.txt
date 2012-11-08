
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# checking whether we should enable activities support

macro_optional_find_package(KActivities 6.1.0)
macro_log_feature(KActivities_FOUND "KActivities" "Activities interface library" "https://projects.kde.org/projects/kde/kdelibs/kactivities" FALSE "6.1.0" "Required for Activities integration.")

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

target_link_libraries(okular ${KDE4_KPARTS_LIBS} ${KACTIVITIES_LIBRARY})

install(TARGETS okular ${INSTALL_TARGETS_DEFAULT_ARGS})


# okular shell data files

install( PROGRAMS okular.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES shell.rc DESTINATION ${DATA_INSTALL_DIR}/okular )

