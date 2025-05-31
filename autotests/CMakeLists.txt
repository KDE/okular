# define which offscreen platform plugin to use for tests
set(OFFSCREEN_QPA)
#set(OFFSCREEN_QPA "-platform" "offscreen")

add_definitions( -DKDESRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/" )

include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)

# wrapper to add a test and run it with the right QPA
macro(add_okular_unittest _source ${ARGN})
    get_filename_component(_name ${_source} NAME_WE)
    add_executable(${_name} ${_source} ${ARGN})
    add_test(NAME ${_name} COMMAND ${_name} ${OFFSCREEN_QPA})
    ecm_mark_as_test(${_name})
    target_link_libraries(${_name}
        okularcore
        Qt6::Test
        Qt6::Widgets
        Qt6::Xml
        KF6::Completion
        KF6::I18n
        KF6::ThreadWeaver
        KF6::WidgetsAddons
    )
    if (BUILD_DESKTOP)
        target_link_libraries(${_name} okularpart)
    endif()
    if (HAVE_X11)
        target_link_libraries(${_name} Qt6::GuiPrivate)
    endif()
endmacro ()

add_okular_unittest(editdrawingtooldialogtest.cpp ../part/editdrawingtooldialog.cpp)

add_okular_unittest(shelltest.cpp ../shell/shellutils.cpp)

if(Poppler_Qt6_FOUND)
    if (BUILD_DESKTOP)
        add_okular_unittest(parttest.cpp closedialoghelper.cpp)
    endif()

    add_okular_unittest(visibilitytest.cpp)
    add_okular_unittest(jsfunctionstest.cpp)
    add_okular_unittest(formattest.cpp)
    add_okular_unittest(keystroketest.cpp)
    add_okular_unittest(signunsignedfieldtest.cpp)
endif()

add_okular_unittest(suggestedfilenametest.cpp)
add_okular_unittest(documenttest.cpp)
add_okular_unittest(searchtest.cpp)
add_okular_unittest(annotationstest.cpp)
add_okular_unittest(urldetecttest.cpp)
add_okular_unittest(editannotationcontentstest.cpp testingutils.cpp)
add_okular_unittest(addremoveannotationtest.cpp testingutils.cpp)
add_okular_unittest(translateannotationtest.cpp testingutils.cpp)
add_okular_unittest(modifyannotationpropertiestest.cpp testingutils.cpp)
add_okular_unittest(editformstest.cpp)
add_okular_unittest(calculatetexttest.cpp)
add_okular_unittest(check_distinguished_name_parser.cpp)

if(PlasmaActivities_FOUND AND BUILD_DESKTOP)
    add_okular_unittest(mainshelltest.cpp ../shell/okular_main.cpp ../shell/shellutils.cpp ../shell/shell.cpp ../shell/welcomescreen.cpp ../shell/recentitemsmodel.cpp closedialoghelper.cpp)
    target_compile_definitions(mainshelltest PRIVATE OKULAR_BINARY="$<TARGET_FILE:okular>")
    target_link_libraries(mainshelltest Plasma::Activities)
endif()

if(BUILD_DESKTOP)
    add_okular_unittest(annotationtoolbartest.cpp ../shell/okular_main.cpp ../shell/shellutils.cpp ../shell/shell.cpp ../shell/welcomescreen.cpp ../shell/recentitemsmodel.cpp closedialoghelper.cpp ../shell/welcomescreen.ui)
endif()

add_okular_unittest(generatorstest.cpp)
target_compile_definitions(generatorstest PRIVATE GENERATORS_BUILD_DIR="${CMAKE_BINARY_DIR}/generators")

add_okular_unittest(signatureformtest.cpp)

find_package(Discount)
if(Discount_FOUND)
    add_okular_unittest(markdowntest.cpp ../generators/markdown/converter.cpp)
    target_link_libraries(markdowntest PkgConfig::Discount)
endif()

add_okular_unittest(toggleactionmenutest.cpp ../part/toggleactionmenu.cpp)
