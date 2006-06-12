
########### next target ###############

set(okular_SRCS main.cpp shell.cpp )

kde4_automoc(${okular_SRCS})

kde4_add_executable(okular ${okular_SRCS})

target_link_libraries(okular  ${KDE4_KDECORE_LIBS} ${KDE4_KPARTS_LIBS} )

install_targets(/bin okular )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES  okular.desktop )
install_files( ${DATA_INSTALL_DIR}/okular FILES  shell.rc )



#original Makefile.am contents follow:

#INCLUDES = -I$(top_builddir)/kpdf $(all_includes)
#
#METASOURCES = AUTO
#
#bin_PROGRAMS = okular
#
#okular_SOURCES = main.cpp shell.cpp
#okular_LDFLAGS = $(KDE_RPATH) $(all_libraries)
#okular_LDADD   = $(LIB_KPARTS)
#
#EXTRA_DIST = okular.desktop
#xdg_apps_DATA = okular.desktop
#
#shellrcdir   = $(kde_datadir)/okular
#shellrc_DATA = shell.rc
