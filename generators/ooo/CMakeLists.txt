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

target_link_libraries(okularGenerator_ooo okularcore KF5::KIOCore KF5::Archive KF5::I18n KF5::Wallet KF5::KDELibs4Support ${MATH_LIB} Qt5::Xml)
if (Qca-qt5_FOUND)
  target_link_libraries(okularGenerator_ooo okularcore qca-qt5)
endif ()

install(TARGETS okularGenerator_ooo DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### install files ###############

install( FILES libokularGenerator_ooo.desktop okularOoo.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_ooo.desktop active-documentviewer_ooo.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )

