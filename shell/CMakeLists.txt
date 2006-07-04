
########### next target ###############

set(okular_SRCS main.cpp shell.cpp )

kde4_automoc(${okular_SRCS})

kde4_add_executable(okular ${okular_SRCS})

target_link_libraries(okular  ${KDE4_KDECORE_LIBS} ${KDE4_KPARTS_LIBS} )

install(TARGETS okular DESTINATION bin)

########### install files ###############

install( FILES okular.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES shell.rc DESTINATION ${DATA_INSTALL_DIR}/okular )



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
