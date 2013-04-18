include_directories(
   ${CMAKE_BINARY_DIR}/okular
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
   ${CMAKE_CURRENT_SOURCE_DIR}/../../generators/plucker/unpluck
   ${JPEG_INCLUDE_DIR}
   ${ZLIB_INCLUDE_DIR}
)

set(qunpluck_SRCS
    unpluck/config.cpp
    unpluck/image.cpp
    unpluck/qunpluck.cpp
    unpluck/unpluck.cpp
    unpluck/util.cpp
)

########### next target ###############

set(okularGenerator_plucker_SRCS
  generator_plucker.cpp
)


kde4_add_plugin(okularGenerator_plucker ${okularGenerator_plucker_SRCS} ${qunpluck_SRCS})

target_link_libraries(okularGenerator_plucker okularcore ${KDE4_KIO_LIBS} ${MATH_LIB} ${JPEG_LIBRARY} ${ZLIB_LIBRARY})

install(TARGETS okularGenerator_plucker DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_plucker.desktop okularPlucker.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_plucker.desktop active-documentviewer_plucker.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

