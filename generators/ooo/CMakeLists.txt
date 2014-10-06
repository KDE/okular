include_directories(
   ${CMAKE_BINARY_DIR}/okular
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

find_package(QCA2 "2.0.0")
set_package_properties("QCA" PROPERTIES
        DESCRIPTION  "Qt Cryptographic Architecture (QCA)"
        URL "http://www.digikam.org/sharedlibs"
        TYPE RECOMMENDED
        PURPOSE "Support for encrypted OpenDocument Text documents in Okular.")


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


add_library(okularGenerator_ooo MODULE ${okularGenerator_ooo_PART_SRCS})

target_link_libraries(okularGenerator_ooo okularcore KF5::KIOCore ${MATH_LIB} Qt5::Xml)
if (QCA2_FOUND)
  target_link_libraries(okularGenerator_ooo okularcore ${QCA2_LIBRARIES})
endif (QCA2_FOUND)

install(TARGETS okularGenerator_ooo DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_ooo.desktop okularOoo.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_ooo.desktop active-documentviewer_ooo.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

