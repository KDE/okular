include_directories(
   ${CMAKE_SOURCE_DIR}/okular
)


########### next target ###############

set(okularGenerator_xps_SRCS
   generator_xps.cpp
)

kde4_automoc(okularGenerator_xps ${okularGenerator_xps_SRCS})

kde4_add_plugin(okularGenerator_xps WITH_PREFIX ${okularGenerator_xps_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} okularGenerator_xps )

target_link_libraries(okularGenerator_xps okularcore ${KDE4_KDEUI_LIBS} )

install(TARGETS okularGenerator_xps DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_xps.desktop okularXps.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

