include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
)


########### next target ###############

set( okularGenerator_comicbook_PART_SRCS
     document.cpp
     generator_comicbook.cpp
     directory.cpp
     unrar.cpp qnatsort.cpp
     unrarflavours.cpp
   )


add_library(okularGenerator_comicbook MODULE ${okularGenerator_comicbook_PART_SRCS})

target_link_libraries(okularGenerator_comicbook  okularcore KF5::KIOCore)
if (UNIX)
   find_package(KF5 REQUIRED Pty)
   target_link_libraries(okularGenerator_comicbook KF5::Pty )
endif (UNIX)

install(TARGETS okularGenerator_comicbook DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_comicbook.desktop okularComicbook.desktop DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_comicbook.desktop active-documentviewer_comicbook.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

