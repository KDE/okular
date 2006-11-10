include_directories(
   ${CMAKE_SOURCE_DIR}/okular
)


########### next target ###############

set(okularGenerator_kimgio_PART_SRCS generator_kimgio.cpp )

kde4_automoc(${okularGenerator_kimgio_PART_SRCS})

kde4_add_plugin(okularGenerator_kimgio WITH_PREFIX ${okularGenerator_kimgio_PART_SRCS})

target_link_libraries(okularGenerator_kimgio  okularcore ${KDE4_KDEUI_LIBS} )

install(TARGETS okularGenerator_kimgio DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_kimgio.desktop okularKimgio.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

