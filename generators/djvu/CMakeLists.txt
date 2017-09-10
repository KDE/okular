remove_definitions(-DTRANSLATION_DOMAIN="okular")
add_definitions(-DTRANSLATION_DOMAIN="okular_djvu")

include_directories(
   ${DJVULIBRE_INCLUDE_DIR}
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
)


########### next target ###############

set(okularGenerator_djvu_SRCS
   generator_djvu.cpp
   kdjvu.cpp
)


okular_add_generator(okularGenerator_djvu ${okularGenerator_djvu_SRCS})

target_link_libraries(okularGenerator_djvu okularcore ${DJVULIBRE_LIBRARY} KF5::I18n)

########### install files ###############
install( FILES okularDjvu.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_djvu.desktop org.kde.mobile.okular_djvu.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES org.kde.okular-djvu.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )

