
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

okular_add_generator(okularGenerator_epub ${okularGenerator_epub_PART_SRCS})

target_link_libraries(okularGenerator_epub okularcore ${EPUB_LIBRARIES} KF5::I18n Qt5::Widgets Qt5::Xml)

########### install files ###############
install( FILES okularEPub.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_epub.desktop org.kde.mobile.okular_epub.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
