INCLUDES = -I$(srcdir)/.. -I$(top_builddir)/kpdf $(all_includes)

METASOURCES = AUTO

libkpdfui_la_SOURCES = pageview.cpp pageviewutils.cpp thumbnaillist.cpp \
                       toc.cpp searchwidget.cpp \
                       propertiesdialog.cpp presentationwidget.cpp \
                       minibar.cpp

noinst_LTLIBRARIES = libkpdfui.la

pageview.lo: ../conf/settings.h
pageviewutils.lo: ../conf/settings.h
presentationwidget.lo: ../conf/settings.h
searchwidget.lo: ../conf/settings.h
thumbnaillist.lo: ../conf/settings.h

