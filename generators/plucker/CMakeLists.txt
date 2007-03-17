include_directories(
   ${CMAKE_BINARY_DIR}/okular
   ${CMAKE_SOURCE_DIR}/okular
   ${CMAKE_SOURCE_DIR}/okular/generators/plucker/unpluck
   ${JPEG_INCLUDE_DIR}
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

kde4_automoc(${okularGenerator_plucker_SRCS})

kde4_add_plugin(okularGenerator_plucker WITH_PREFIX ${okularGenerator_plucker_SRCS} ${qunpluck_SRCS})

target_link_libraries(okularGenerator_plucker okularcore ${KDE4_KDEPRINT_LIBS} m ${JPEG_LIBRARY} )

install(TARGETS okularGenerator_plucker DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_plucker.desktop okularPlucker.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES x-plucker.desktop DESTINATION ${MIME_INSTALL_DIR}/application )
