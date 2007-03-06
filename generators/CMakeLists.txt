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


# this is a crude hack to prevent breakage while I modify the XPS code
# to use Qt4.3 streaming parsers - bradh.
find_path( QT_XMLSTREAMREADER QXmlStreamReader ${QT_QTXML_INCLUDE_DIR} )
if (${QT_XMLSTREAMREADER} STREQUAL ${QT_QTXML_INCLUDE_DIR})
   add_subdirectory(xps)
endif(${QT_XMLSTREAMREADER} STREQUAL ${QT_QTXML_INCLUDE_DIR})

add_subdirectory(ooo)

add_subdirectory(fictionbook)

add_subdirectory(comicbook)
