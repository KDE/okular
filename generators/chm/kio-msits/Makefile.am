METASOURCES = AUTO
INCLUDES = $(QT_INCLUDES) -I$(CHM_INCLUDES) $(KDE_INCLUDES)

kde_module_LTLIBRARIES = kio_msits.la

kio_msits_la_SOURCES = msits.cpp
kio_msits_la_LIBADD = -lkio -lchm
kio_msits_la_LDFLAGS = -module -avoid-version $(QT_LIBS) $(KDE_LDFLAGS) $(KDE_LIBS) $(KDE_PLUGIN)

protocol_DATA = msits.protocol
protocoldir = $(kde_servicesdir)


