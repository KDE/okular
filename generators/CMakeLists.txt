# let's enable the generators properly configured

if(POPPLER_FOUND)
  add_subdirectory(poppler)
endif(POPPLER_FOUND)

if(LIBGS_FOUND)
  add_subdirectory(ghostview)
endif(LIBGS_FOUND)

add_subdirectory( kimgio )

if(CHM_FOUND)
  add_subdirectory( chm )
endif(CHM_FOUND)

if(DJVULIBRE_FOUND)
  add_subdirectory(djvu)
endif(DJVULIBRE_FOUND)

add_subdirectory(dvi)

if(TIFF_FOUND)
  add_subdirectory(tiff)
endif(TIFF_FOUND)

add_subdirectory(xps)

add_subdirectory(ooo)

add_subdirectory(fictionbook)

add_subdirectory(comicbook)
