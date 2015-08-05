include_directories(
   ${DJVULIBRE_INCLUDE_DIR}
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
   ${CMAKE_BINARY_DIR}/okular
)


########### next target ###############

set(okularGenerator_djvu_SRCS
   generator_djvu.cpp
   kdjvu.cpp
)


add_library(okularGenerator_djvu MODULE ${okularGenerator_djvu_SRCS})

target_link_libraries(okularGenerator_djvu okularcore ${DJVULIBRE_LIBRARY} KF5::KDELibs4Support)

install(TARGETS okularGenerator_djvu DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### install files ###############

install( FILES libokularGenerator_djvu.desktop okularDjvu.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_djvu.desktop org.kde.mobile.okular_djvu.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )


