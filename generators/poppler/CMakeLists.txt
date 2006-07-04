include_directories(
   ${POPPLER_INCLUDE_DIR}/poppler
   ${CMAKE_BINARY_DIR}/okular
   ${CMAKE_SOURCE_DIR}/okular
)


########### next target ###############

set(okularGenerator_poppler_PART_SRCS generator_pdf.cpp )

kde4_automoc(${okularGenerator_poppler_PART_SRCS})

kde4_add_plugin(okularGenerator_poppler WITH_PREFIX ${okularGenerator_poppler_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} okularGenerator_poppler )

target_link_libraries(okularGenerator_poppler   ${POPPLER_LIBRARY} ${KDE4_KDECORE_LIBS} okularcore ${KDE4_KDEPRINT_LIBS} ${KDE4_KDEUI_LIBS} m )

install(TARGETS okularGenerator_poppler DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_poppler.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

