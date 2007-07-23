add_subdirectory( kio-msits )

include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
   ${CMAKE_BINARY_DIR}/okular
   ${CHM_INCLUDE_DIR}
   ${CMAKE_CURRENT_SOURCE_DIR}/../../generators/chm/lib
)


########### next target ###############

set(chmlib_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/../../generators/chm/lib/iconstorage.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/../../generators/chm/lib/kchmtextencoding.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/../../generators/chm/lib/kchmtreeviewitem.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/../../generators/chm/lib/kchmurl.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/../../generators/chm/lib/xchmfile.cpp
)

set(okularGenerator_chmlib_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/../../generators/chm/generator_chm.cpp
)

set(okularGenerator_chmlib_PART_SRCS ${chmlib_SRCS} ${okularGenerator_chmlib_SRCS} )


kde4_add_plugin(okularGenerator_chmlib WITH_PREFIX ${okularGenerator_chmlib_PART_SRCS})

target_link_libraries(okularGenerator_chmlib  okularcore ${CHM_LIBRARY} ${KDE4_KHTML_LIBS} )

install(TARGETS okularGenerator_chmlib DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_chmlib.desktop okularChm.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES okularApplication_chm.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

