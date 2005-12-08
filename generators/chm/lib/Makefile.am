noinst_LTLIBRARIES = libchmfile.la

INCLUDES = -I$(srcdir)/../.. -I$(top_builddir)/kpdf $(all_includes)

libchmfile_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) 
libchmfile_la_LIBADD = $(LIB_KDEUI) $(LIB_KIO) $(LIB_QT) -lchm 
libchmfile_la_SOURCES = kchmtextencoding.cpp xchmfile.cpp iconstorage.cpp kchmurl.cpp kchmtreeviewitem.cpp

