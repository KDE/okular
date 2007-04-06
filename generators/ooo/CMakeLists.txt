include_directories(
   ${CMAKE_BINARY_DIR}/okular
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
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

target_link_libraries(okularGenerator_ooo   ${POPPLER_LIBRARY} okularcore ${KDE4_KDEPRINT_LIBS} ${MATH_LIB} )

install(TARGETS okularGenerator_ooo DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_ooo.desktop okularOoo.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES okularApplication_ooo.desktop  DESTINATION  ${XDG_APPS_DIR} )

