
macro_optional_find_package(ActiveApp)
macro_log_feature(ACTIVEAPP_FOUND "ActiveApp" "Support for developing applications for Plasma Active" FALSE "" "")

if (ACTIVEAPP_FOUND)
    add_subdirectory( app )
endif (ACTIVEAPP_FOUND)

add_subdirectory( components )
 
