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

target_link_libraries(okularGenerator_fb okularcore ${KDE4_KIO_LIBS} ${QT_QTXML_LIBRARY})

install(TARGETS okularGenerator_fb DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_fb.desktop okularFb.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_fb.desktop active-documentviewer_fb.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
