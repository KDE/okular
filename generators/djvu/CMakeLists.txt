include_directories(
   ${DJVULIBRE_INCLUDE_DIR}
   ${CMAKE_SOURCE_DIR}/okular
   ${CMAKE_BINARY_DIR}/okular
)


########### next target ###############

set(okularGenerator_djvu_SRCS
   generator_djvu.cpp
   kdjvu.cpp
)

kde4_automoc(${okularGenerator_djvu_SRCS})

kde4_add_plugin(okularGenerator_djvu WITH_PREFIX ${okularGenerator_djvu_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} okularGenerator_djvu )

target_link_libraries(okularGenerator_djvu ${KDE4_KDECORE_LIBS} okularcore ${DJVULIBRE_LIBRARY} ${KDE4_KDEPRINT_LIBS} ${KDE4_KDEUI_LIBS} )

install_targets(${PLUGIN_INSTALL_DIR} okularGenerator_djvu )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES  libokularGenerator_djvu.desktop )


