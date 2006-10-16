
########### next target ###############

set(okular_SRCS main.cpp shell.cpp )

kde4_automoc(okular ${okular_SRCS})

kde4_add_executable(okular ${okular_SRCS})

target_link_libraries(okular  ${KDE4_KPARTS_LIBS} ${KDE4_KDE3SUPPORT_LIBRARY} )

install(TARGETS okular DESTINATION ${BIN_INSTALL_DIR})

########### install files ###############

install( FILES okular.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES shell.rc DESTINATION ${DATA_INSTALL_DIR}/okular )


