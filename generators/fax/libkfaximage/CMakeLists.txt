kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### next target ###############

set(kfaximage_LIB_SRCS kfaximage.cpp faxexpand.cpp faxinit.cpp )

kde4_automoc(${kfaximage_LIB_SRCS})

kde4_add_library(kfaximage SHARED ${kfaximage_LIB_SRCS})

target_link_libraries(kfaximage  ${KDE4_KDECORE_LIBS} )

set_target_properties(kfaximage PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install_targets(${LIB_INSTALL_DIR} kfaximage )


########### install files ###############

install_files( /include FILES  kfaximage.h )

kde4_footer()



#original Makefile.am contents follow:

#INCLUDES = -I$(top_srcdir) $(all_includes) 
#
#lib_LTLIBRARIES = libkfaximage.la
#libkfaximage_la_LDFLAGS = $(all_libraries) -no-undefined -avoid-version
#libkfaximage_la_LIBADD  = $(LIB_QTCORE) $(LIB_QT) $(LIB_KDECORE)
#libkfaximage_la_SOURCES = kfaximage.cpp faxexpand.cpp faxinit.cpp
#
#include_HEADERS = kfaximage.h
#noinst_HEADERS = faxexpand.h
#
#METASOURCES = AUTO
#
#messages: rc.cpp
#	$(XGETTEXT) $(libkfaximage_la_SOURCES) -o $(podir)/libkfaximgage.pot
