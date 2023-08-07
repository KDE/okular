add_definitions( -DKDESRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/" )

include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)

macro(add_conf_unittest _source)
  set(_test ${_source} ../part/editdrawingtooldialog.cpp)
  get_filename_component(_name ${_source} NAME_WE)
  add_executable( ${_name} ${_test} )
  add_test( NAME ${_name} COMMAND ${_name} )
  ecm_mark_as_test(${_name})
  target_link_libraries( ${_name}
    Qt5::Test
    Qt5::Widgets
    Qt5::Xml
    KF5::I18n
    KF5::Completion
    KF5::WidgetsAddons
  )
endmacro ()

add_conf_unittest(editdrawingtooldialogtest.cpp)

ecm_add_test(shelltest.cpp ../shell/shellutils.cpp
    TEST_NAME "shelltest"
    LINK_LIBRARIES Qt5::Test okularcore
)

if(Poppler_Qt5_FOUND)
    if (BUILD_DESKTOP)
        ecm_add_test(parttest.cpp closedialoghelper.cpp
            TEST_NAME "parttest"
            LINK_LIBRARIES Qt5::Widgets Qt5::Test Qt5::Xml okularcore okularpart
        )
    endif()

    ecm_add_test(visibilitytest.cpp
        TEST_NAME "visibilitytest"
        LINK_LIBRARIES Qt5::Widgets Qt5::Test okularcore
    )

    ecm_add_test(jsfunctionstest.cpp
        TEST_NAME "jsfunctionstest"
        LINK_LIBRARIES Qt5::Widgets Qt5::Test okularcore
    )

    ecm_add_test(formattest.cpp
        TEST_NAME "formattest"
        LINK_LIBRARIES Qt5::Widgets Qt5::Test okularcore
    )

    ecm_add_test(keystroketest.cpp
        TEST_NAME "keystroketest"
        LINK_LIBRARIES Qt5::Widgets Qt5::Test okularcore
    )

    ecm_add_test(signunsignedfieldtest.cpp
        TEST_NAME "signunsignedfieldtest"
        LINK_LIBRARIES Qt5::Widgets Qt5::Test okularcore
    )
endif()

ecm_add_test(suggestedfilenametest.cpp
    TEST_NAME "suggestedfilenametest"
    LINK_LIBRARIES Qt5::Test okularcore KF5::I18n
)

ecm_add_test(documenttest.cpp
    TEST_NAME "documenttest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test Qt5::Xml okularcore KF5::ThreadWeaver
)

ecm_add_test(searchtest.cpp
    TEST_NAME "searchtest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test Qt5::Xml okularcore
)

ecm_add_test(annotationstest.cpp
    TEST_NAME "annotationstest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test Qt5::Xml okularcore
)

ecm_add_test(urldetecttest.cpp
    TEST_NAME "urldetecttest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test Qt5::Xml KF5::CoreAddons
)

ecm_add_test(editannotationcontentstest.cpp testingutils.cpp
    TEST_NAME "editannotationcontentstest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test Qt5::Xml okularcore
)

ecm_add_test(addremoveannotationtest.cpp testingutils.cpp
    TEST_NAME "addremoveannotationtest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test Qt5::Xml okularcore
)

ecm_add_test(translateannotationtest.cpp testingutils.cpp
    TEST_NAME "translateannotationtest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test Qt5::Xml okularcore
)

ecm_add_test(modifyannotationpropertiestest.cpp testingutils.cpp
    TEST_NAME "modifyannotationpropertiestest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test okularcore
)

ecm_add_test(editformstest.cpp
    TEST_NAME "editformstest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test okularcore
)

ecm_add_test(calculatetexttest.cpp
    TEST_NAME "calculatetexttest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test okularcore
)

if(KF5Activities_FOUND AND BUILD_DESKTOP)
	ecm_add_test(mainshelltest.cpp ../shell/okular_main.cpp ../shell/shellutils.cpp ../shell/shell.cpp ../shell/welcomescreen.cpp ../shell/recentitemsmodel.cpp closedialoghelper.cpp
		TEST_NAME "mainshelltest"
		LINK_LIBRARIES Qt5::Test KF5::Activities okularpart okularcore
	)
	target_compile_definitions(mainshelltest PRIVATE OKULAR_BINARY="$<TARGET_FILE:okular>")

	if (HAVE_X11)
        target_link_libraries(mainshelltest Qt5::X11Extras)
    endif()
endif()

if(BUILD_DESKTOP)
    ecm_add_test(annotationtoolbartest.cpp ../shell/okular_main.cpp ../shell/shellutils.cpp ../shell/shell.cpp ../shell/welcomescreen.cpp ../shell/recentitemsmodel.cpp closedialoghelper.cpp ../shell/welcomescreen.ui
        TEST_NAME "annotationtoolbartest"
        LINK_LIBRARIES Qt5::Test okularpart
    )

    if (HAVE_X11)
        target_link_libraries(annotationtoolbartest Qt5::X11Extras)
    endif()
endif()

ecm_add_test(generatorstest.cpp
    TEST_NAME "generatorstest"
    LINK_LIBRARIES Qt5::Test KF5::CoreAddons okularcore
)
target_compile_definitions(generatorstest PRIVATE GENERATORS_BUILD_DIR="${CMAKE_BINARY_DIR}/generators")

ecm_add_test(signatureformtest.cpp
    TEST_NAME "signatureformtest"
    LINK_LIBRARIES Qt5::Test okularcore
)

find_package(Discount)

if(Discount_FOUND)
    ecm_add_test(markdowntest.cpp ../generators/markdown/converter.cpp
        TEST_NAME "markdowntest"
        LINK_LIBRARIES Qt5::Test okularcore KF5::I18n PkgConfig::Discount
    )
endif()

ecm_add_test(toggleactionmenutest.cpp ../part/toggleactionmenu.cpp
    TEST_NAME "toggleactionmenutest"
    LINK_LIBRARIES Qt5::Test KF5::WidgetsAddons
)
