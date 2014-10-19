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


add_library(okularGenerator_fb MODULE ${okularGenerator_fb_PART_SRCS})

target_link_libraries(okularGenerator_fb okularcore KF5::KIOCore Qt5::Xml)

install(TARGETS okularGenerator_fb DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_fb.desktop okularFb.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_fb.desktop active-documentviewer_fb.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

ecm_install_icons(ICONS hi16-apps-okular-fb2.png  hi32-apps-okular-fb2.png  hi48-apps-okular-fb2.png DESTINATION ${DATA_INSTALL_DIR}/okular/icons)
