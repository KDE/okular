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

target_link_libraries(okularGenerator_txt okularcore Qt5::Core KF5::I18n)
install(TARGETS okularGenerator_txt DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### install files ###############

install( FILES libokularGenerator_txt.desktop okularTxt.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_txt.desktop active-documentviewer_txt.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
