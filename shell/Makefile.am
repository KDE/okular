INCLUDES = -I$(top_builddir)/kpdf $(all_includes)

METASOURCES = AUTO

bin_PROGRAMS = oKular

oKular_SOURCES = main.cpp shell.cpp
oKular_LDFLAGS = $(KDE_RPATH) $(all_libraries)
oKular_LDADD   = $(LIB_KPARTS)

EXTRA_DIST = oKular.desktop
xdg_apps_DATA = oKular.desktop

shellrcdir   = $(kde_datadir)/oKular
shellrc_DATA = shell.rc
