#add_subdirectory( kio-msits )

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

add_library(okularGenerator_chmlib MODULE ${okularGenerator_chmlib_SRCS})

target_link_libraries(okularGenerator_chmlib  okularcore ${CHM_LIBRARY} KF5::KHtml)

install(TARGETS okularGenerator_chmlib DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### install files ###############

install( FILES libokularGenerator_chmlib.desktop okularChm.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_chm.desktop active-documentviewer_chm.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )

