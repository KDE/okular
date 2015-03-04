include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
)


########### next target ###############

set(okularGenerator_xps_SRCS
   generator_xps.cpp
)


add_library(okularGenerator_xps MODULE ${okularGenerator_xps_SRCS})

target_link_libraries(okularGenerator_xps okularcore KF5::KIOCore Qt5::Xml)

install(TARGETS okularGenerator_xps DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### install files ###############

install( FILES libokularGenerator_xps.desktop okularXps.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_xps.desktop active-documentviewer_xps.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )

