include_directories(
   ${CMAKE_BINARY_DIR}/okular
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

find_package(Qca-qt5 2.1.0)
set_package_properties(Qca-qt5 PROPERTIES DESCRIPTION "Support for encryption"
                       URL "http://download.kde.org/stable/qca-qt5/"
                       TYPE RECOMMENDED)

if ( Qca-qt5_FOUND )
   add_definitions( -DQCA2 )
endif()

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
if (Qca-qt5_FOUND)
  target_link_libraries(okularGenerator_ooo okularcore qca-qt5)
endif ()

install(TARGETS okularGenerator_ooo DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES libokularGenerator_ooo.desktop okularOoo.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_ooo.desktop active-documentviewer_ooo.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )

