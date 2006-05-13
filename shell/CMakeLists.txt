
########### next target ###############

set(oKular_SRCS main.cpp shell.cpp )

kde4_automoc(${oKular_SRCS})

kde4_add_executable(oKular ${oKular_SRCS})

target_link_libraries(oKular  ${KDE4_KDECORE_LIBS} ${KDE4_KPARTS_LIBS} )

install_targets(/bin oKular )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES  oKular.desktop )
install_files( ${DATA_INSTALL_DIR}/oKular FILES  shell.rc )



#original Makefile.am contents follow:

#INCLUDES = -I$(top_builddir)/kpdf $(all_includes)
#
#METASOURCES = AUTO
#
#bin_PROGRAMS = oKular
#
#oKular_SOURCES = main.cpp shell.cpp
#oKular_LDFLAGS = $(KDE_RPATH) $(all_libraries)
#oKular_LDADD   = $(LIB_KPARTS)
#
#EXTRA_DIST = oKular.desktop
#xdg_apps_DATA = oKular.desktop
#
#shellrcdir   = $(kde_datadir)/oKular
#shellrc_DATA = shell.rc
