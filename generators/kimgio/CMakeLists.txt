include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
   ${CMAKE_BINARY_DIR}
   ${QIMAGEBLITZ_INCLUDES}
   ${KEXIV2_INCLUDE_DIR}
)


########### next target ###############

set(okularGenerator_kimgio_PART_SRCS generator_kimgio.cpp )


kde4_add_plugin(okularGenerator_kimgio ${okularGenerator_kimgio_PART_SRCS})
target_link_libraries(okularGenerator_kimgio okularcore ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${QT_QTXML_LIBRARY} ${KEXIV2_LIBRARIES} )
install(TARGETS okularGenerator_kimgio DESTINATION ${PLUGIN_INSTALL_DIR})

set( kimgiotest_SRCS tests/kimgiotest.cpp ${CMAKE_SOURCE_DIR}/ui/pagepainter.cpp ${CMAKE_SOURCE_DIR}/ui/guiutils.cpp )
kde4_add_kcfg_files( kimgiotest_SRCS ${CMAKE_SOURCE_DIR}/conf/settings.kcfgc )
kde4_add_unit_test( kimgiotest ${kimgiotest_SRCS} )
target_link_libraries( kimgiotest ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} ${QIMAGEBLITZ_LIBRARIES} okularcore )


########### install files ###############

install( FILES libokularGenerator_kimgio.desktop okularKimgio.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_kimgio.desktop active-documentviewer_kimgio.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

#install( FILES gui.rc DESTINATION ${DATA_INSTALL_DIR}/okular_kimgio )
