include_directories(
   ${CMAKE_SOURCE_DIR}/okular
   ${CHM_INCLUDE_DIR}
)


########### next target ###############

set(kio_msits_PART_SRCS msits.cpp )

kde4_automoc(kio_msits ${kio_msits_PART_SRCS})

kde4_add_plugin(kio_msits ${kio_msits_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_msits )

target_link_libraries(kio_msits  ${KDE4_KIO_LIBS} ${CHM_LIBRARY} )

install(TARGETS kio_msits DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES msits.protocol DESTINATION  ${SERVICES_INSTALL_DIR} )

