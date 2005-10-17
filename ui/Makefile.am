SUBDIRS = data painter_agg2 .

INCLUDES = -I$(srcdir)/.. -I$(srcdir)/painter_agg2 -I$(top_builddir)/kpdf $(all_includes)

METASOURCES = AUTO

noinst_LTLIBRARIES = liboKularui.la
liboKularui_la_SOURCES = pagepainter.cpp pageview.cpp pageviewutils.cpp \
                       pageviewannotator.cpp minibar.cpp thumbnaillist.cpp \
                       searchwidget.cpp toc.cpp propertiesdialog.cpp \
                       presentationwidget.cpp side_reviews.cpp newstuff.cpp

pageview.lo: ../conf/settings.h
pageviewutils.lo: ../conf/settings.h
presentationwidget.lo: ../conf/settings.h
searchwidget.lo: ../conf/settings.h
thumbnaillist.lo: ../conf/settings.h

