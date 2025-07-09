set(CMAKE_AUTORCC ON)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../..
  ${CMAKE_CURRENT_BINARY_DIR}/../../
)
add_executable(okularkirigami main.cpp)
target_link_libraries(okularkirigami PRIVATE Qt6::Widgets Qt6::Qml KF6::I18n KF6::CoreAddons)

ecm_add_qml_module(okularkirigami
    URI "org.kde.okular.app"
    GENERATE_PLUGIN_SOURCE
)

ecm_target_qml_sources(okularkirigami SOURCES
    ui/Bookmarks.qml
    ui/CertificateViewerPage.qml
    ui/Main.qml
    ui/MainView.qml
    ui/OkularDrawer.qml
    ui/TableOfContents.qml
    ui/Thumbnails.qml
    ui/ThumbnailsBase.qml
    ui/TreeItem.qml
    ui/TreeViewDecoration.qml
    ui/Signatures.qml
    ui/SignaturePropertiesPage.qml
    ui/WelcomeView.qml
)

if (ANDROID)
    target_sources(okularkirigami PRIVATE android.cpp)

    # androiddeployqt only packages libraries needed by the main binary.
    # That is problematic because some of the libraries we need we only use in plugins
    # here we help androiddeployqt by linking those plugins dependencies
    # to the app.
    get_property(aux_list GLOBAL PROPERTY okular_generator_list)
    foreach(okular_generator ${aux_list})
      get_target_property(okular_generator_dependencies ${okular_generator} LINK_LIBRARIES)
      target_link_libraries(okularkirigami PRIVATE ${okular_generator_dependencies})
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
        target_link_libraries(okularkirigami PRIVATE -lsoftokn3 -lnssckbi -lfreebl3)
    endif()

    kirigami_package_breeze_icons(ICONS
        application-pkcs7-signature
        bookmark-remove
        bookmarks-organize
        dialog-close
        document-open
        help-about-symbolic
        okular
        view-preview
        view-table-of-contents-ltr
    )

    ecm_add_android_apk(okularkirigami ANDROID_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../android)
endif()

install(TARGETS okularkirigami ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES package/metadata.desktop DESTINATION ${KDE_INSTALL_APPDIR} RENAME org.kde.okular.kirigami.desktop)
install( FILES org.kde.okular.kirigami.appdata.xml DESTINATION  ${KDE_INSTALL_METAINFODIR} )
