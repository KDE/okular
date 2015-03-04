include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
   ${TIFF_INCLUDE_DIR}
)


########### next target ###############

set(okularGenerator_tiff_SRCS
   generator_tiff.cpp
)

add_library(okularGenerator_tiff MODULE ${okularGenerator_tiff_SRCS})
target_link_libraries(okularGenerator_tiff okularcore ${TIFF_LIBRARIES})
install(TARGETS okularGenerator_tiff DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### install files ###############

install( FILES libokularGenerator_tiff.desktop okularTiff.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_tiff.desktop active-documentviewer_tiff.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
