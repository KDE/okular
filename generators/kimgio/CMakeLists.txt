include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
   ${CMAKE_BINARY_DIR}
)


########### next target ###############

set(okularGenerator_kimgio_PART_SRCS generator_kimgio.cpp )
add_library(okularGenerator_kimgio MODULE ${okularGenerator_kimgio_PART_SRCS})
target_link_libraries(okularGenerator_kimgio okularcore KF5::KExiv2)
install(TARGETS okularGenerator_kimgio DESTINATION ${PLUGIN_INSTALL_DIR})

add_definitions( -DKDESRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/" )
set( kimgiotest_SRCS tests/kimgiotest.cpp ${CMAKE_SOURCE_DIR}/ui/pagepainter.cpp ${CMAKE_SOURCE_DIR}/ui/guiutils.cpp )
ecm_add_test(${kimgiotest_SRCS} TEST_NAME "kimgiotest" LINK_LIBRARIES okularcore okularpart Qt5::Svg Qt5::Test)

########### install files ###############

install( FILES libokularGenerator_kimgio.desktop okularKimgio.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS okularApplication_kimgio.desktop active-documentviewer_kimgio.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
