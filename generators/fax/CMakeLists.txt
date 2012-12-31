include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
)


########### next target ###############

set(okularGenerator_fax_PART_SRCS generator_fax.cpp faxdocument.cpp faxexpand.cpp faxinit.cpp )

kde4_add_plugin(okularGenerator_fax ${okularGenerator_fax_PART_SRCS})

target_link_libraries(okularGenerator_fax okularcore ${KDE4_KDEUI_LIBS} ${QT_QTXML_LIBRARY})

install(TARGETS okularGenerator_fax DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_fax.desktop okularFax.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_fax.desktop active-documentviewer_fax.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
