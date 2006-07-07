add_subdirectory( conf ) 

include_directories(
   ${CMAKE_SOURCE_DIR}/okular
   ${CMAKE_CURRENT_BINARY_DIR}/../..
   ${CMAKE_SOURCE_DIR}/libqgs
)

add_definitions(${KDE4_ENABLE_EXCEPTIONS})

########### next target ###############

set(okulargsasyncgenerator_SRCS
   okulargsasyncgenerator.cpp
)

kde4_automoc(${okulargsasyncgenerator_SRCS})

kde4_add_executable(okulargsasyncgenerator ${okulargsasyncgenerator_SRCS})

target_link_libraries(okulargsasyncgenerator  ${KDE4_KDECORE_LIBS} qgs )

install(TARGETS okulargsasyncgenerator DESTINATION bin)


########### next target ###############

set(okularGenerator_ghostview_SRCS
   interpreter_cmd.cpp
   internaldocument.cpp
   generator_ghostview.cpp
   gvlogwindow.cpp
   dscparse_adapter.cpp
   ps.c
   dscparse.cpp
)

kde4_add_ui_files(okularGenerator_ghostview_SRCS
   conf/gssettingswidget.ui
)

kde4_add_kcfg_files(okularGenerator_ghostview_SRCS conf/gssettings.kcfgc )

kde4_automoc(${okularGenerator_ghostview_SRCS})

kde4_add_plugin(okularGenerator_ghostview WITH_PREFIX ${okularGenerator_ghostview_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} okularGenerator_ghostview )

target_link_libraries(okularGenerator_ghostview  ${KDE4_KDECORE_LIBS} okularcore qgs ${KDE4_KDEUI_LIBRARY} ${KDE4_KDEPRINT_LIBRARY} )

install(TARGETS okularGenerator_ghostview DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_ghostview.desktop okularGhostview.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

