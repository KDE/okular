find_package(KDE4 REQUIRED)
include(KDE4Defaults)

find_package(Okular REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(EPub REQUIRED)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(
   ${KDE4_INCLUDES}
   ${OKULAR_INCLUDE_DIR}
   ${EPUB_INCLUDE_DIR}
)


########### next target ###############

set(okularGenerator_epub_PART_SRCS
  converter.cpp
  generator_epub.cpp
)

kde4_add_plugin(okularGenerator_epub ${okularGenerator_epub_PART_SRCS})

target_link_libraries(okularGenerator_epub ${OKULAR_LIBRARIES} ${EPUB_LIBRARIES} ${KDE4_KIO_LIBS} ${MATH_LIB})

install(TARGETS okularGenerator_epub DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_epub.desktop okularEPub.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES okularApplication_epub.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
