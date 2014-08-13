include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
   ${CMAKE_BINARY_DIR}/okular
)

########### next target ###############

set(okularGenerator_txt_SRCS
   generator_txt.cpp
   converter.cpp
   document.cpp
)


add_library(okularGenerator_txt MODULE ${okularGenerator_txt_SRCS})

target_link_libraries(okularGenerator_txt okularcore Qt5::Core)
kservice_desktop_to_json(okularGenerator_txt libokularGenerator_txt.desktop)

install(TARGETS okularGenerator_txt DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES okularTxt.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_txt.desktop active-documentviewer_txt.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
