include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
)


########### next target ###############

set(okularGenerator_fax_PART_SRCS generator_fax.cpp faxdocument.cpp faxexpand.cpp faxinit.cpp fax_debug.cpp)

add_library(okularGenerator_fax MODULE ${okularGenerator_fax_PART_SRCS})

target_link_libraries(okularGenerator_fax okularcore  Qt5::Xml)

install(TARGETS okularGenerator_fax DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### install files ###############

install( FILES libokularGenerator_fax.desktop okularFax.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_fax.desktop active-documentviewer_fax.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
