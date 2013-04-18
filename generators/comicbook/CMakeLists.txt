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


kde4_add_plugin(okularGenerator_comicbook ${okularGenerator_comicbook_PART_SRCS})

target_link_libraries(okularGenerator_comicbook  okularcore ${KDE4_KIO_LIBS} )
if (UNIX)
   target_link_libraries(okularGenerator_comicbook ${KDE4_KPTY_LIBRARY})
endif (UNIX)

install(TARGETS okularGenerator_comicbook DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_comicbook.desktop okularComicbook.desktop DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_comicbook.desktop active-documentviewer_comicbook.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

