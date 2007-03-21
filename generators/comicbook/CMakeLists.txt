include_directories(
   ${CMAKE_SOURCE_DIR}/okular
)


########### next target ###############

set( okularGenerator_comicbook_PART_SRCS
     document.cpp
     generator_comicbook.cpp
     unrar.cpp
   )

kde4_automoc(${okularGenerator_comicbook_PART_SRCS})

kde4_add_plugin(okularGenerator_comicbook WITH_PREFIX ${okularGenerator_comicbook_PART_SRCS})

target_link_libraries(okularGenerator_comicbook  okularcore ${KDE4_KDEUI_LIBS} )

install(TARGETS okularGenerator_comicbook DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_comicbook.desktop okularComicbook.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

