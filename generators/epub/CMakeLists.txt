
add_subdirectory(data)

include_directories(
   ${EPUB_INCLUDE_DIR}
)


########### next target ###############

set(okularGenerator_epub_PART_SRCS
  converter.cpp
  epubdocument.cpp
  generator_epub.cpp
)

add_library(okularGenerator_epub MODULE ${okularGenerator_epub_PART_SRCS})

target_link_libraries(okularGenerator_epub okularcore ${EPUB_LIBRARIES} KF5::KDELibs4Support Qt5::Widgets Qt5::Xml)

install(TARGETS okularGenerator_epub DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### install files ###############

install( FILES libokularGenerator_epub.desktop okularEPub.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_epub.desktop active-documentviewer_epub.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
