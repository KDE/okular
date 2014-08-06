
find_package(ActiveApp "0.1")
set_package_properties("ActiveApp" PROPERTIES
    DESCRIPTION "Support for developing applications for Plasma Active"
    URL "https://projects.kde.org/projects/extragear/base/plasma-mobile"
    TYPE OPTIONAL)

if (ACTIVEAPP_FOUND)
    add_subdirectory( app )
endif (ACTIVEAPP_FOUND)

add_subdirectory( components )
 
