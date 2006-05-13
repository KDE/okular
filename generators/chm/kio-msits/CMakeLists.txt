include_directories(
   ${CMAKE_SOURCE_DIR}/okular
   ${CHM_INCLUDE_DIR}
)


########### next target ###############

set(kio_msits_PART_SRCS msits.cpp )

kde4_automoc(${kio_msits_PART_SRCS})

kde4_add_plugin(kio_msits ${kio_msits_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_msits )

target_link_libraries(kio_msits  ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${CHM_LIBRARY} )

install_targets(${PLUGIN_INSTALL_DIR} kio_msits )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES  msits.protocol )



#original Makefile.am contents follow:

#METASOURCES = AUTO
#INCLUDES = $(QT_INCLUDES) -I$(CHM_INCLUDES) $(KDE_INCLUDES) $(all_includes)
#
#kde_module_LTLIBRARIES = kio_msits.la
#
#kio_msits_la_SOURCES = msits.cpp
#kio_msits_la_LIBADD = $(LIB_KIO) -lchm
#kio_msits_la_LDFLAGS = -module -avoid-version $(QT_LIBS) $(KDE_LDFLAGS) $(KDE_LIBS) $(KDE_PLUGIN)
#
#protocol_DATA = msits.protocol
#protocoldir = $(kde_servicesdir)
#
#
