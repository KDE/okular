CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )

find_package(KDE4 REQUIRED)

macro_optional_find_package(TIFF)

include_directories(
   ${KDE4_INCLUDES} ${QT_INCLUDES}
   ${CMAKE_CURRENT_BINARY_DIR}
   ${TIFF_INCLUDE_DIR}
)


########### next target ###############

set(okularGenerator_tiff_SRCS
   generator_tiff.cpp
)


kde4_add_plugin(okularGenerator_tiff ${okularGenerator_tiff_SRCS})

target_link_libraries(okularGenerator_tiff okularcore ${TIFF_LIBRARIES} ${KDE4_KDEUI_LIBS} ${QT_QTXML_LIBRARY})

install(TARGETS okularGenerator_tiff DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_tiff.desktop okularTiff.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES okularApplication_tiff.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

