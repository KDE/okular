
macro_optional_find_package(ActiveApp)
macro_log_feature(ACTIVEAPP_FOUND "ActiveApp" "Support for developing applications for Plasma Active" "https://projects.kde.org/projects/extragear/base/plasma-mobile" FALSE "0.1")

if (ACTIVEAPP_FOUND)
    add_subdirectory( app )
endif (ACTIVEAPP_FOUND)

add_subdirectory( components )
 
