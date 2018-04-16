remove_definitions(-DTRANSLATION_DOMAIN="okular")
add_definitions(-DTRANSLATION_DOMAIN="okular_ooo")

include_directories(
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


okular_add_generator(okularGenerator_ooo ${okularGenerator_ooo_PART_SRCS})

target_link_libraries(okularGenerator_ooo okularcore KF5::KIOCore KF5::Archive KF5::I18n  ${MATH_LIB} Qt5::Xml)
if (Qca-qt5_FOUND)
  target_link_libraries(okularGenerator_ooo qca-qt5)
endif ()
if (KF5Wallet_FOUND)
  target_link_libraries(okularGenerator_ooo KF5::Wallet)
endif ()

########### install files ###############
install( FILES okularOoo.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_ooo.desktop org.kde.mobile.okular_ooo.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES org.kde.okular-ooo.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
