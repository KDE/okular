



########### next target ###############

set(kpdf_SRCS main.cpp shell.cpp )

kde4_automoc(${kpdf_SRCS})

kde4_add_executable(kpdf ${kpdf_SRCS})

target_link_libraries(kpdf  ${KDE4_KPARTS_LIBS} )

install_targets(/bin kpdf )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES kpdf.desktop )
install_files( ${DATA_INSTALL_DIR}/kpdf FILES shell.rc )




#original Makefile.am contents follow:

#INCLUDES = -I$(top_builddir)/kpdf $(all_includes) 
#
#METASOURCES = AUTO
#
#bin_PROGRAMS = kpdf
#
#kpdf_SOURCES = main.cpp shell.cpp
#kpdf_LDFLAGS = $(KDE_RPATH) $(all_libraries)
#kpdf_LDADD   = $(LIB_KPARTS)
#
#EXTRA_DIST = kpdf.desktop
#xdg_apps_DATA = kpdf.desktop
#
#shellrcdir   = $(kde_datadir)/kpdf
#shellrc_DATA = shell.rc
