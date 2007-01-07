include_directories(
   ${CMAKE_BINARY_DIR}/okular
   ${CMAKE_SOURCE_DIR}/okular
)


########### next target ###############

set(okularGenerator_fb_PART_SRCS
  converter.cpp
  document.cpp
  generator_fb.cpp
)

kde4_automoc(${okularGenerator_fb_PART_SRCS})

kde4_add_plugin(okularGenerator_fb WITH_PREFIX ${okularGenerator_fb_PART_SRCS})

target_link_libraries(okularGenerator_fb ${POPPLER_LIBRARY} okularcore ${KDE4_KDEPRINT_LIBS} m )

install(TARGETS okularGenerator_fb DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_fb.desktop okularFb.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
