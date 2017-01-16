
include_directories(  ${QT_INCLUDES} )

#add_subdirectory(src)

# QML-only thing
#install(DIRECTORY package/ DESTINATION ${KDE_INSTALL_DATADIR}/plasma/packages/org.kde.active.documentviewer)

install(DIRECTORY package/ DESTINATION ${KDE_INSTALL_DATADIR}/kpackage/genericqml/org.kde.mobile.okular)
install(FILES package/metadata.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} RENAME org.kde.mobile.okular.desktop)


#install(FILES active-documentviewer.desktop DESTINATION ${KDE_INSTALL_APPDIR})

