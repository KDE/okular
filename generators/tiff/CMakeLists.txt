include_directories(
   ${CMAKE_SOURCE_DIR}/okular
   ${TIFF_INCLUDE_DIR}
)


########### next target ###############

set(okularGenerator_tiff_SRCS
   generator_tiff.cpp
)

kde4_automoc(${okularGenerator_tiff_SRCS})

kde4_add_plugin(okularGenerator_tiff WITH_PREFIX ${okularGenerator_tiff_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} okularGenerator_tiff )

target_link_libraries(okularGenerator_tiff   okularcore ${TIFF_LIBRARIES} ${KDE4_KDEUI_LIBS} )

install(TARGETS okularGenerator_tiff DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_tiff.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

