include_directories(
   ${DJVULIBRE_INCLUDE_DIR}
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
   ${CMAKE_BINARY_DIR}/okular
)


########### next target ###############

set(okularGenerator_djvu_SRCS
   generator_djvu.cpp
   kdjvu.cpp
)


add_library(okularGenerator_djvu MODULE ${okularGenerator_djvu_SRCS})

target_link_libraries(okularGenerator_djvu okularcore ${DJVULIBRE_LIBRARY} KF5::KDELibs4Support Qt5::Gui Qt5::PrintSupport)

install(TARGETS okularGenerator_djvu DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_djvu.desktop okularDjvu.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_djvu.desktop active-documentviewer_djvu.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )


