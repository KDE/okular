include (MacroLogFeature)

macro_optional_find_package(Poppler)
macro_optional_find_package(LIBGS)
macro_optional_find_package(CHM)
macro_optional_find_package(DjVuLibre)
macro_optional_find_package(TIFF)
macro_optional_find_package(Freetype)
macro_optional_find_package(JPEG)

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

if(JPEG_FOUND)
  add_subdirectory(plucker)
endif(JPEG_FOUND)

macro_log_feature(POPPLER_FOUND "Poppler-Qt4" "Support for PDF files in okular" "http://poppler.freedesktop.org" FALSE "0.5.4" "")
macro_log_feature(LIBGS_FOUND "libgs" "Support for PS files in okular" "http://www.cs.wisc.edu/~ghost" FALSE "" "")
macro_log_feature(CHM_FOUND "libCHM" "Support for CHM files in okular" "http://morte.jedrea.com/~jedwin/projects/chmlib" FALSE "" "")
macro_log_feature(DJVULIBRE_FOUND "libdjvulibre" "Support for DjVu files in okular" "http://djvulibre.djvuzone.org" FALSE "3.5.17" "")
macro_log_feature(TIFF_FOUND "libTIFF" "Support for TIFF files in okular" "http://www.remotesensing.org/libtiff/" FALSE "" "")
macro_log_feature(JPEG_FOUND "JPEG" "Support for PalmDB docments in okular" "http://www.ijg.org/" FALSE "" "")
