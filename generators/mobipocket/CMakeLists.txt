include_directories( ${QMOBIPOCKET_INCLUDE_DIR}  )

########### next target ###############

set(okularGenerator_mobi_PART_SRCS
  mobidocument.cpp
  generator_mobi.cpp
  converter.cpp
)

kde4_add_plugin(okularGenerator_mobi ${okularGenerator_mobi_PART_SRCS})

target_link_libraries(okularGenerator_mobi okularcore ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTGUI_LIBRARY} ${QMOBIPOCKET_LIBRARIES})

install(TARGETS okularGenerator_mobi DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install( FILES libokularGenerator_mobi.desktop okularMobi.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_mobi.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
