include_directories(
   ${CMAKE_BINARY_DIR}/okular
   ${CMAKE_SOURCE_DIR}/okular
)


########### next target ###############

set(okularGenerator_ooo_PART_SRCS
  converter.cpp
  document.cpp
  formatproperty.cpp
  generator_ooo.cpp
  styleinformation.cpp
  styleparser.cpp
)

kde4_automoc(${okularGenerator_ooo_PART_SRCS})

kde4_add_plugin(okularGenerator_ooo WITH_PREFIX ${okularGenerator_ooo_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} okularGenerator_ooo )

target_link_libraries(okularGenerator_ooo   ${POPPLER_LIBRARY} okularcore ${KDE4_KDEPRINT_LIBS} m )

install(TARGETS okularGenerator_ooo DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_ooo.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

