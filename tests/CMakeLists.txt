set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

kde4_add_unit_test( shelltest shelltest.cpp ../shell/shellutils.cpp )
target_link_libraries( shelltest ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} )
