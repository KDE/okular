include_directories(
   ${CMAKE_BINARY_DIR}/okular
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
)


########### next target ###############

set(okularGenerator_fb_PART_SRCS
  converter.cpp
  document.cpp
  generator_fb.cpp
)


kde4_add_plugin(okularGenerator_fb ${okularGenerator_fb_PART_SRCS})

target_link_libraries(okularGenerator_fb okularcore ${KDE4_KDEUI_LIBS})

install(TARGETS okularGenerator_fb DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_fb.desktop okularFb.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES okularApplication_fb.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
