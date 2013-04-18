include_directories(
   ${CMAKE_BINARY_DIR}/okular
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

macro_optional_find_package(QCA2)
macro_log_feature(QCA2_FOUND "QCA" "Qt Cryptographic Architecture (QCA)" "http://delta.affinix.com/qca/" FALSE "2.0.0" "Support for encrypted OpenDocument Text documents in Okular.")

if ( QCA2_FOUND )
   include_directories( ${QCA2_INCLUDE_DIR} )
   add_definitions( -DQCA2 )
endif( QCA2_FOUND )

########### next target ###############

set(okularGenerator_ooo_PART_SRCS
  converter.cpp
  document.cpp
  formatproperty.cpp
  generator_ooo.cpp
  manifest.cpp
  styleinformation.cpp
  styleparser.cpp
)


kde4_add_plugin(okularGenerator_ooo ${okularGenerator_ooo_PART_SRCS})

target_link_libraries(okularGenerator_ooo okularcore ${KDE4_KIO_LIBS} ${MATH_LIB} ${QT_QTXML_LIBRARY})
if (QCA2_FOUND)
  target_link_libraries(okularGenerator_ooo okularcore ${QCA2_LIBRARIES})
endif (QCA2_FOUND)

install(TARGETS okularGenerator_ooo DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_ooo.desktop okularOoo.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_ooo.desktop active-documentviewer_ooo.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

