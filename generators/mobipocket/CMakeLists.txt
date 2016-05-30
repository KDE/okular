include_directories( ${QMOBIPOCKET_INCLUDE_DIR}  )

########### next target ###############

set(okularGenerator_mobi_PART_SRCS
  mobidocument.cpp
  generator_mobi.cpp
  converter.cpp
)

okular_add_generator(okularGenerator_mobi ${okularGenerator_mobi_PART_SRCS})

target_link_libraries(okularGenerator_mobi okularcore Qt5::Gui ${QMOBIPOCKET_LIBRARIES})

########### install files ###############

install( PROGRAMS okularApplication_mobi.desktop org.kde.mobile.okular_mobi.desktop DESTINATION  ${KDE_INSTALL_APPDIR} )
