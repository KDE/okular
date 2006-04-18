kde4_header()

add_subdirectory( pix ) 

include_directories( ${CMAKE_SOURCE_DIR}/kviewshell/shell ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(kdvipart_PART_SRCS 
   bigEndianByteReader.cpp 
   dviexport.cpp 
   dviFile.cpp 
   dviPageCache.cpp 
   dviRenderer.cpp 
   dviRenderer_draw.cpp 
   dviRenderer_prescan.cpp 
   dvisourceeditor.cpp 
   dvisourcesplitter.cpp 
   dviWidget.cpp 
   fontEncoding.cpp 
   fontEncodingPool.cpp 
   fontMap.cpp 
   fontpool.cpp 
   fontprogress.cpp 
   glyph.cpp 
   infodialog.cpp 
   kdvi_multipage.cpp 
   kdvi_multipage_texthandling.cpp 
   optionDialogFontsWidget.cpp 
   optionDialogSpecialWidget.cpp 
   psgs.cpp 
   psheader.cpp 
   renderedDviPagePixmap.cpp 
   special.cpp 
   TeXFont.cpp 
   TeXFont_PFB.cpp 
   TeXFont_PK.cpp 
   TeXFont_TFM.cpp 
   TeXFontDefinition.cpp 
   util.cpp 
   vf.cpp )

kde4_automoc(${kdvipart_PART_SRCS})

kde4_add_ui_files(kdvipart_PART_SRCS optionDialogFontsWidget_base.ui optionDialogSpecialWidget_base.ui )

kde4_add_kcfg_files(kdvipart_PART_SRCS prefs.kcfgc )

kde4_add_plugin(kdvipart ${kdvipart_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kdvipart )

target_link_libraries(kdvipart  ${KDE4_KDECORE_LIBS} kparts kmultipage )

install_targets(${PLUGIN_INSTALL_DIR} kdvipart )


########### next target ###############

set(squeeze_SRCS squeeze.c )

kde4_automoc(${squeeze_SRCS})

kde4_add_executable(squeeze ${squeeze_SRCS})

target_link_libraries(squeeze  ${KDE4_KDECORE_LIBS} )


########### install files ###############

install_files( ${DATA_INSTALL_DIR} FILES kdvi_part.rc )
install_files( ${KCFG_INSTALL_DIR} FILES kdvi.kcfg )
install_files( ${DATA_INSTALL_DIR}/kdvi FILES tips )
install_files( ${SERVICES_INSTALL_DIR} FILES kdvimultipage.desktop )

kde4_footer()



#original Makefile.am contents follow:

## set the include path for X, qt and KDE
#INCLUDES= -I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/kviewshell/shell \
#	-I$(top_builddir)/kviewshell/shell \
#	$(all_includes) $(LIBFREETYPE_CFLAGS)
## claim, which subdirectories you want to install
#SUBDIRS = . pix
#
## you can add here more. This one gets installed
#kde_module_LTLIBRARIES= kdvipart.la
#noinst_PROGRAMS = squeeze
#
## just to make sure, automake makes them
#METASOURCES = AUTO
#
#kdvipart_la_SOURCES = \
#	bigEndianByteReader.cpp \
#	dviexport.cpp \
#	dviFile.cpp \
#	dviPageCache.cpp \
#	dviRenderer.cpp \
#	dviRenderer_draw.cpp \
#	dviRenderer_prescan.cpp \
#	dvisourceeditor.cpp \
#	dvisourcesplitter.cpp \
#	dviWidget.cpp \
#	fontEncoding.cpp \
#	fontEncodingPool.cpp \
#	fontMap.cpp \
#	fontpool.cpp \
#	fontprogress.cpp \
#	glyph.cpp \
#	infodialog.cpp \
#	kdvi_multipage.cpp \
#	kdvi_multipage_texthandling.cpp \
#	optionDialogFontsWidget.cpp \
#	optionDialogFontsWidget_base.ui \
#	optionDialogSpecialWidget.cpp \
#	optionDialogSpecialWidget_base.ui \
#	prefs.kcfgc \
#	psgs.cpp \
#	psheader.cpp \
#	renderedDviPagePixmap.cpp \
#	special.cpp \
#	TeXFont.cpp \
#	TeXFont_PFB.cpp \
#	TeXFont_PK.cpp \
#	TeXFont_TFM.cpp \
#	TeXFontDefinition.cpp \
#	util.cpp \
#	vf.cpp
#
#kde_kcfg_DATA = kdvi.kcfg
#
#kdvipart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
#kdvipart_la_LIBADD = $(LIBFREETYPE_LIBS) -lkparts \
#			$(top_builddir)/kviewshell/shell/libkmultipage.la
#
## Which sources should be compiled for squeeze.
#squeeze_SOURCES	= squeeze.c
#
#KDE_OPTIONS = nofinal
#
### this option you can leave out. Just, if you use "make dist", you need it
#noinst_HEADERS = dvi.h dviRenderer.h xdvi.h
#
#messages: rc.cpp
#	$(PREPARETIPS) > tips.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/kdvi.pot
#	rm -f tips.cpp
#
#tip_DATA = tips
#tipdir = $(kde_datadir)/kdvi
#
#plugindir = $(kde_datadir)
#plugin_DATA = kdvi_part.rc
#
#kde_services_DATA = kdvimultipage.desktop
#
#psheader.h: psheader.cpp
#
#psheader.cpp: psheader.txt squeeze$(EXEEXT)
#	./squeeze$(EXEEXT) $(srcdir)/psheader.txt psheader.h psheader.cpp
#
#CLEANFILES = psheader.cpp psheader.h
