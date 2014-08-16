########### install files ###############
#
#
kdoctools_create_handbook(index.docbook INSTALL_DESTINATION ${HTML_INSTALL_DIR}/en SUBDIR okular)
kdoctools_create_manpage(man-okular.1.docbook 1 INSTALL_DESTINATION ${MAN_INSTALL_DIR})