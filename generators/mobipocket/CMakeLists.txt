include_directories( ${QMOBIPOCKET_INCLUDE_DIR}  )

########### next target ###############

set(okularGenerator_mobi_PART_SRCS
  mobidocument.cpp
  generator_mobi.cpp
  converter.cpp
)

add_library(okularGenerator_mobi MODULE ${okularGenerator_mobi_PART_SRCS})

target_link_libraries(okularGenerator_mobi okularcore KF5::KDELibs4Support  Qt5::Gui ${QMOBIPOCKET_LIBRARIES})

install(TARGETS okularGenerator_mobi DESTINATION ${KDE_INSTALL_PLUGINDIR})

########### install files ###############

install( FILES libokularGenerator_mobi.desktop okularMobi.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_mobi.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
