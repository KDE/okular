add_subdirectory( conf ) 

include_directories(
   ${LIBSPECTRE_INCLUDE_DIR}
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
   ${CMAKE_CURRENT_BINARY_DIR}/../..
)

########### next target ###############

set(okularGenerator_ghostview_SRCS
   generator_ghostview.cpp
   rendererthread.cpp
   spectre_debug.cpp
)

ki18n_wrap_ui(okularGenerator_ghostview_SRCS
   conf/gssettingswidget.ui
)

kconfig_add_kcfg_files(okularGenerator_ghostview_SRCS conf/gssettings.kcfgc )


add_library(okularGenerator_ghostview MODULE ${okularGenerator_ghostview_SRCS})

target_link_libraries(okularGenerator_ghostview okularcore ${LIBSPECTRE_LIBRARY} KF5::KDELibs4Support Qt5::Xml Qt5::PrintSupport)

install(TARGETS okularGenerator_ghostview DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### install files ###############

install( FILES libokularGenerator_ghostview.desktop okularGhostview.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_ghostview.desktop active-documentviewer_ghostview.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )

ecm_install_icons(ICONS hi16-apps-okular-gv.png  hi32-apps-okulars-gv.png DESTINATION ${KDE_INSTALL_DATADIR}/okular/icons)
