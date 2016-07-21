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


okular_add_generator(okularGenerator_fb ${okularGenerator_fb_PART_SRCS})

target_link_libraries(okularGenerator_fb okularcore KF5::Archive KF5::I18n KF5::KIOCore Qt5::Xml)

########### install files ###############
install( FILES okularFb.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_fb.desktop org.kde.mobile.okular_fb.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )

ecm_install_icons(ICONS 16-apps-okular-fb2.png  32-apps-okular-fb2.png  48-apps-okular-fb2.png DESTINATION ${KDE_INSTALL_DATADIR}/okular/icons THEME hicolor)
