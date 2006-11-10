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

target_link_libraries(okularGenerator_djvu okularcore ${DJVULIBRE_LIBRARY} ${KDE4_KDEPRINT_LIBS} ${KDE4_KDEUI_LIBS} )

install(TARGETS okularGenerator_djvu DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_djvu.desktop okularDjvu.desktop DESTINATION  ${SERVICES_INSTALL_DIR} )


