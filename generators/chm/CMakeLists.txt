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

kde4_add_plugin(okularGenerator_chmlib ${okularGenerator_chmlib_SRCS})

target_link_libraries(okularGenerator_chmlib  okularcore ${CHM_LIBRARY} ${KDE4_KHTML_LIBS} )

install(TARGETS okularGenerator_chmlib DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_chmlib.desktop okularChm.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_chm.desktop active-documentviewer_chm.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

