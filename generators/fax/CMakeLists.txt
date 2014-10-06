include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
)


########### next target ###############

set(okularGenerator_fax_PART_SRCS generator_fax.cpp faxdocument.cpp faxexpand.cpp faxinit.cpp )

add_library(okularGenerator_fax MODULE ${okularGenerator_fax_PART_SRCS})

target_link_libraries(okularGenerator_fax okularcore  Qt5::Xml)

install(TARGETS okularGenerator_fax DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_fax.desktop okularFax.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_fax.desktop active-documentviewer_fax.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
