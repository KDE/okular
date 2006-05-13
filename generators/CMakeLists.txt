# let's enable the generators properly configured

if(POPPLER_FOUND)
  add_subdirectory(poppler)
endif(POPPLER_FOUND)
if(QGS_FOUND)
  add_subdirectory(ghostview)
endif(QGS_FOUND)
add_subdirectory( fax )
add_subdirectory( kimgio )
if(CHM_FOUND)
  add_subdirectory( chm )
endif(CHM_FOUND)

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
