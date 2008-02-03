include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../../..
   ${CMAKE_CURRENT_SOURCE_DIR}/../lib
   ${CHM_INCLUDE_DIR}
)


########### next target ###############

set(kio_msits_PART_SRCS msits.cpp )


kde4_add_plugin(kio_msits ${kio_msits_PART_SRCS})

target_link_libraries(kio_msits  ${KDE4_KIO_LIBS} ${CHM_LIBRARY} )

install(TARGETS kio_msits DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES msits.protocol DESTINATION  ${SERVICES_INSTALL_DIR} )

