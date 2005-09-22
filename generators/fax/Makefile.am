INCLUDES = -I$(top_srcdir)/kviewshell \
	-I$(top_builddir)/kviewshell \
	-I$(kde_includes)/kviewshell \
	-I$(srcdir)/libkfaximage \
	-I$(top_srcdir) $(all_includes)

SUBDIRS = libkfaximage .

KDE_ICON = kfaxview

METASOURCES = AUTO

bin_PROGRAMS = kfaxview
kfaxview_SOURCES = main.cpp
kfaxview_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kfaxview_LDADD = ../kviewshell/libifaces.la ../kviewshell/libkviewshell.la -lkparts

# this is where the desktop file will go
kde_services_DATA = kfaxmultipage.desktop kfaxmultipage_tiff.desktop

# this is where the shell's XML-GUI resource file goes
shellrcdir = $(kde_datadir)/kfaxview

kde_module_LTLIBRARIES = kfaxviewpart.la
kfaxviewpart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
kfaxviewpart_la_LIBADD = -lkdeprint -lkparts $(top_builddir)/kviewshell/libkmultipage.la libkfaximage/libkfaximage.la
kfaxviewpart_la_SOURCES = faxmultipage.cpp faxrenderer.cpp

partdir = $(kde_datadir)/kfaxview
part_DATA = ../kviewshell/kviewshell.rc

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kfaxview.pot

xdg_apps_DATA = kfaxview.desktop

# The next line switches the API documentation on
include ../admin/Doxyfile.am
