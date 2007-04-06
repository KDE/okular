include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
   ${TIFF_INCLUDE_DIR}
)


########### next target ###############

set(okularGenerator_tiff_SRCS
   generator_tiff.cpp
)

kde4_automoc(${okularGenerator_tiff_SRCS})

kde4_add_plugin(okularGenerator_tiff WITH_PREFIX ${okularGenerator_tiff_SRCS})

target_link_libraries(okularGenerator_tiff   okularcore ${TIFF_LIBRARIES} ${KDE4_KDEUI_LIBS} ${KDE4_KDEPRINT_LIBS} )

install(TARGETS okularGenerator_tiff DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_tiff.desktop okularTiff.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES okularApplication_tiff.desktop  DESTINATION  ${XDG_APPS_DIR} )

