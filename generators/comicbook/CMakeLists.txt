remove_definitions(-DTRANSLATION_DOMAIN="okular")
add_definitions(-DTRANSLATION_DOMAIN="okular_comicbook")

include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

########### next target ###############

set( okularGenerator_comicbook_PART_SRCS
     document.cpp
     generator_comicbook.cpp
     directory.cpp
     unrar.cpp qnatsort.cpp
     unrarflavours.cpp
   )


okular_add_generator(okularGenerator_comicbook ${okularGenerator_comicbook_PART_SRCS})
target_link_libraries(okularGenerator_comicbook okularcore KF5::KIOCore KF5::I18n KF5::Archive)
if (UNIX AND NOT ANDROID)
   find_package(KF5Pty REQUIRED)
   target_compile_definitions(okularGenerator_comicbook PRIVATE -DWITH_KPTY=1)
   target_link_libraries(okularGenerator_comicbook KF5::Pty)
endif ()

if (KArchive_HAVE_LZMA)
    target_compile_definitions(okularGenerator_comicbook PRIVATE -DWITH_K7ZIP=1)
endif()

########### install files ###############
install( FILES okularComicbook.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_comicbook.desktop org.kde.mobile.okular_comicbook.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES org.kde.okular-comicbook.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
