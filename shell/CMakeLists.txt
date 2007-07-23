
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# okular

set(okular_SRCS
   main.cpp
   shell.cpp
)


kde4_add_executable(okular ${okular_SRCS})

target_link_libraries(okular  ${KDE4_KPARTS_LIBS} )

install(TARGETS okular DESTINATION ${BIN_INSTALL_DIR})


# okular shell data files

install( FILES okular.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES shell.rc DESTINATION ${DATA_INSTALL_DIR}/okular )

