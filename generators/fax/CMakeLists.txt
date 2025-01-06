remove_definitions(-DTRANSLATION_DOMAIN="okular")
add_definitions(-DTRANSLATION_DOMAIN="okular_fax")

########### next target ###############

set(okularGenerator_fax_PART_SRCS generator_fax.cpp faxdocument.cpp faxexpand.cpp faxinit.cpp fax_debug.cpp)

okular_add_generator(okularGenerator_fax ${okularGenerator_fax_PART_SRCS})

target_link_libraries(okularGenerator_fax okularcore KF6::I18n Qt6::Xml)

########### install files ###############
if (BUILD_DESKTOP)
  install( PROGRAMS okularApplication_fax.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
endif()
if (BUILD_MOBILE)
  install( PROGRAMS org.kde.mobile.okular_fax.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
endif()
install( FILES org.kde.okular-fax.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
