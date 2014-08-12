set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
add_definitions( -DKDESRCDIR="${CMAKE_CURRENT_SOURCE_DIR}" )

ecm_add_test(shelltest shelltest.cpp ../shell/shellutils.cpp
    TEST_NAME "shelltest"
    LINK_LIBRARIES Qt5::Test okularcore KF5::KDELibs4Support
)

ecm_add_test(parttest parttest.cpp
    TEST_NAME "parttest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test Qt5::Xml okularcore okularpart KF5::KDELibs4Support
)

#ecm_add_test(documenttest documenttest.cpp
#    TEST_NAME "documenttest"
#    LINK_LIBRARIES Qt5::Widgets Qt5::Test Qt5::Xml okularcore KF5::KDELibs4Support
#)

ecm_add_test(searchtest searchtest.cpp
    TEST_NAME "searchtest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test Qt5::Xml okularcore KF5::KDELibs4Support
)

ecm_add_test(annotationstest annotationstest.cpp
    TEST_NAME "annotationstest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test Qt5::Xml okularcore KF5::KDELibs4Support
)

ecm_add_test(urldetecttest urldetecttest.cpp
    TEST_NAME "urldetecttest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test Qt5::Xml KF5::CoreAddons KF5::KDELibs4Support
)

ecm_add_test(editannotationcontentstest editannotationcontentstest.cpp testingutils.cpp
    TEST_NAME "editannotationcontentstest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test Qt5::Xml okularcore KF5::KDELibs4Support
)

ecm_add_test(addremoveannotationtest addremoveannotationtest.cpp testingutils.cpp
    TEST_NAME "addremoveannotationtest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test Qt5::Xml okularcore KF5::KDELibs4Support
)

ecm_add_test(translateannotationtest translateannotationtest.cpp testingutils.cpp
    TEST_NAME "translateannotationtest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test Qt5::Xml okularcore KF5::KDELibs4Support
)

ecm_add_test(modifyannotationpropertiestest modifyannotationpropertiestest.cpp testingutils.cpp
    TEST_NAME "modifyannotationpropertiestest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test okularcore KF5::KDELibs4Support
)

ecm_add_test(editformstest editformstest.cpp
    TEST_NAME "editformstest"
    LINK_LIBRARIES Qt5::Widgets Qt5::Test okularcore KF5::KDELibs4Support
)
