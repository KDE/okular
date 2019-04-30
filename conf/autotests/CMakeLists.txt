macro(add_conf_unittest _source)
  set(_test ${_source} ../editdrawingtooldialog.cpp)
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

