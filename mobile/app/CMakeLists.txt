set(CMAKE_AUTORCC ON)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../..
  ${CMAKE_CURRENT_BINARY_DIR}/../../
)
add_executable(okularkirigami main.cpp app.qrc)
target_link_libraries(okularkirigami Qt5::Widgets Qt5::Qml KF5::I18n KF5::CoreAddons)
if (ANDROID)
    find_package(Qt5 COMPONENTS AndroidExtras)
    target_sources(okularkirigami PRIVATE android.cpp)
    target_link_libraries(okularkirigami Qt5::AndroidExtras)

    # androiddeployqt only packages libraries needed by the main binary.
    # That is problematic because some of the libraries we need we only use in plugins
    # here we help androiddeployqt by linking those plugins dependencies
    # to the app.
    get_property(aux_list GLOBAL PROPERTY okular_generator_list)
    foreach(okular_generator ${aux_list})
      get_target_property(okular_generator_dependencies ${okular_generator} LINK_LIBRARIES)
      target_link_libraries(okularkirigami ${okular_generator_dependencies})
    endforeach()

    # Unfortunately gitlab CI for now doesn't use Craft so we need
    # to differentiate between gitlab CI and the binary factory
    if (ANDROID_LINK_EXTRA_LIBRARIES)
        message("Linking in extra libraries to help androiddeployqt")
        # androiddeployqt only packages libraries needed by the main binary.
        # the pdf/poppler generator links to poppler that links to NSS
        # and NSS has plugins. Here we link to those plugins so they get packaged
        pkg_check_modules(NSS "nss")
        target_link_directories(okularkirigami PRIVATE ${NSS_LIBRARY_DIRS})
        target_link_libraries(okularkirigami -lsoftokn3 -lnssckbi -lfreebl3)
    endif()

    kirigami_package_breeze_icons(ICONS
        application-pkcs7-signature
        bookmark-remove
        bookmarks-organize
        dialog-close
        document-open
        help-about-symbolic
        view-preview
        view-table-of-contents-ltr
    )
endif()

install(TARGETS okularkirigami ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES package/metadata.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} RENAME org.kde.okular.kirigami.desktop)
install( FILES org.kde.okular.kirigami.appdata.xml DESTINATION  ${KDE_INSTALL_METAINFODIR} )
