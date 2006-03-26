kde4_header()

add_subdirectory( poppler ) 
add_subdirectory( fax ) 
add_subdirectory( kimgio ) 

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(GSPLUGINDIR)")
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(CHMPLUGINDIR)")
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#if include_ghostscript
#GSPLUGINDIR = ghostview
#endif
#
#if include_chm
#CHMPLUGINDIR = chm
#endif
#
#if include_pdf
#PDFPLUGINDIR = poppler
#endif
#
#SUBDIRS = $(GSPLUGINDIR) poppler fax kimgio $(CHMPLUGINDIR)
#
