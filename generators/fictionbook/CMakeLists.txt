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


kde4_add_plugin(okularGenerator_fb WITH_PREFIX ${okularGenerator_fb_PART_SRCS})

target_link_libraries(okularGenerator_fb ${POPPLER_LIBRARY} okularcore ${KDE4_KDEPRINT_LIBS} ${MATH_LIB} )

install(TARGETS okularGenerator_fb DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_fb.desktop okularFb.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES okularApplication_fb.desktop  DESTINATION  ${XDG_APPS_DIR} )
