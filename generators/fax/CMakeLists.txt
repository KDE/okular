add_subdirectory( libkfaximage )

include_directories(
   ${CMAKE_SOURCE_DIR}/okular
   ${CMAKE_BINARY_DIR}/okular
   ${CMAKE_CURRENT_SOURCE_DIR}/libkfaximage
)


########### next target ###############

set(okularGenerator_fax_PART_SRCS faxrenderer.cpp )

kde4_automoc(${okularGenerator_fax_PART_SRCS})

kde4_add_plugin(okularGenerator_fax WITH_PREFIX ${okularGenerator_fax_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} okularGenerator_fax )

target_link_libraries(okularGenerator_fax  ${KDE4_KDECORE_LIBS} okularcore ${KDE4_KDEUI_LIBS} kfaximage )

install(TARGETS okularGenerator_fax DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_fax.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR}  hicolor )

