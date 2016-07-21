add_subdirectory( kio-msits )

include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
   ${CHM_INCLUDE_DIR}
   ${CMAKE_CURRENT_SOURCE_DIR}/lib
)


########### next target ###############

set(okularGenerator_chmlib_SRCS
   lib/lchmurlhandler.cpp
   lib/libchmfile.cpp
   lib/libchmfileimpl.cpp
   lib/libchmfile_search.cpp
   lib/libchmtextencoding.cpp
   lib/libchmtocimage.cpp
   generator_chm.cpp
)

okular_add_generator(okularGenerator_chmlib ${okularGenerator_chmlib_SRCS})
target_link_libraries(okularGenerator_chmlib  okularcore ${CHM_LIBRARY} KF5::KHtml)

########### install files ###############
install( FILES okularChm.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_chm.desktop org.kde.mobile.okular_chm.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )

