include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../../..
   ${CMAKE_CURRENT_SOURCE_DIR}/../lib
   ${CHM_INCLUDE_DIR}
)


########### next target ###############

set(kio_msits_PART_SRCS msits.cpp kio_mits_debug.cpp)


add_library(kio_msits MODULE ${kio_msits_PART_SRCS})

target_link_libraries(kio_msits  KF5::KIOCore Qt5::Core ${CHM_LIBRARY} Qt5::Network)

install(TARGETS kio_msits DESTINATION ${KDE_INSTALL_PLUGINDIR} )


########### install files ###############

install( FILES ms-its.protocol DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )

