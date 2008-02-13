include_directories(
   ${CMAKE_BINARY_DIR}/okular
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
)


########### next target ###############

set(okularGenerator_epub_PART_SRCS
  converter.cpp
  generator_epub.cpp
)


kde4_add_plugin(okularGenerator_epub ${okularGenerator_epub_PART_SRCS})

find_package(LibXml2 REQUIRED)
find_package(LibZip REQUIRED)

include_directories (/usr/local/include ${LIBXML2_INCLUDE_DIR} ${LIBZIP_INCLUDE_DIR})
link_directories (/usr/local/lib)
target_link_libraries(okularGenerator_epub okularcore epub ${KDE4_KIO_LIBS} ${MATH_LIB} )

install(TARGETS okularGenerator_epub DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_epub.desktop okularEPub.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES okularApplication_epub.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
