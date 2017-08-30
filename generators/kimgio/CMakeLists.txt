remove_definitions(-DTRANSLATION_DOMAIN="okular")
add_definitions(-DTRANSLATION_DOMAIN="okular_kimgio")

include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}/../..
   ${CMAKE_BINARY_DIR}
)


########### next target ###############

okular_add_generator(okularGenerator_kimgio generator_kimgio.cpp)
target_link_libraries(okularGenerator_kimgio okularcore KF5::KExiv2 KF5::I18n)

if(BUILD_TESTING)
    add_definitions( -DKDESRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/" )
    set( kimgiotest_SRCS tests/kimgiotest.cpp ${CMAKE_SOURCE_DIR}/ui/pagepainter.cpp ${CMAKE_SOURCE_DIR}/ui/guiutils.cpp ${CMAKE_SOURCE_DIR}/ui/debug_ui.cpp )
    ecm_add_test(${kimgiotest_SRCS} TEST_NAME "kimgiotest" LINK_LIBRARIES okularcore okularpart Qt5::Svg Qt5::Test)
    target_compile_definitions(kimgiotest PRIVATE -DGENERATOR_PATH="$<TARGET_FILE:okularGenerator_kimgio>")
endif()


########### install files ###############
install( FILES okularKimgio.desktop  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS okularApplication_kimgio.desktop org.kde.mobile.okular_kimgio.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES org.kde.okular-kimgio.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
