include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
   ${CMAKE_BINARY_DIR}
)


########### next target ###############

set(okularGenerator_kimgio_PART_SRCS generator_kimgio.cpp )
add_library(okularGenerator_kimgio MODULE ${okularGenerator_kimgio_PART_SRCS})
target_link_libraries(okularGenerator_kimgio okularcore KF5::KExiv2 KF5::KDELibs4Support)
install(TARGETS okularGenerator_kimgio DESTINATION ${KDE_INSTALL_PLUGINDIR})

add_definitions( -DKDESRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/" )
set( kimgiotest_SRCS tests/kimgiotest.cpp ${CMAKE_SOURCE_DIR}/ui/pagepainter.cpp ${CMAKE_SOURCE_DIR}/ui/guiutils.cpp )
ecm_add_test(${kimgiotest_SRCS} TEST_NAME "kimgiotest" LINK_LIBRARIES okularcore okularpart Qt5::Svg Qt5::Test)

########### install files ###############

install( FILES libokularGenerator_kimgio.desktop okularKimgio.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_kimgio.desktop org.kde.mobile.okular_kimgio.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
