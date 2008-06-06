# explicitely install the "public" cmake files

set(cmakeFiles
  FindOkular.cmake
  FindKSane.cmake
)

install(FILES ${cmakeFiles} DESTINATION ${DATA_INSTALL_DIR}/cmake/modules)

