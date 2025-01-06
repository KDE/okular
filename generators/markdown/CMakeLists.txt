remove_definitions(-DTRANSLATION_DOMAIN="okular")
add_definitions(-DTRANSLATION_DOMAIN="okular_markdown")

########### next target ###############

set(okularGenerator_md_PART_SRCS
  converter.cpp
  generator_md.cpp
)


okular_add_generator(okularGenerator_md ${okularGenerator_md_PART_SRCS})

target_link_libraries(okularGenerator_md PRIVATE okularcore KF6::I18n PkgConfig::Discount)

########### install files ###############
if (BUILD_DESKTOP)
  install( PROGRAMS okularApplication_md.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
endif()
if (BUILD_MOBILE)
  install( PROGRAMS org.kde.mobile.okular_md.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
endif()
install( FILES org.kde.okular-md.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )

