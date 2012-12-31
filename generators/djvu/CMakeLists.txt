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


kde4_add_plugin(okularGenerator_djvu ${okularGenerator_djvu_SRCS})

target_link_libraries(okularGenerator_djvu okularcore ${DJVULIBRE_LIBRARY} ${KDE4_KDEUI_LIBS} ${QT_QTXML_LIBRARY})

install(TARGETS okularGenerator_djvu DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_djvu.desktop okularDjvu.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_djvu.desktop active-documentviewer_djvu.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )


